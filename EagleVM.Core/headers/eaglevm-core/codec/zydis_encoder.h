#pragma once
#include <ranges>
#include <queue>

#include "zydis_defs.h"
#include "zydis_enum.h"
#include "zydis_helper.h"
#include "eaglevm-core/compiler/code_label.h"

namespace eagle::codec::encoder
{
    struct mem_op final
    {
        mem_op(const reg base, const int64_t displacement, const uint16_t read_size)
            : base(base), index(none), scale(1), displacement(displacement), read_size(read_size)
        {
        }

        mem_op(const reg base, const int64_t displacement, const reg_size reg_read_size)
            : base(base), index(none), scale(1), displacement(displacement)
        {
            read_size = reg_size_to_b(reg_read_size);
        }

        mem_op(const reg base, const reg index, const uint64_t scale, const int64_t displacement, const uint16_t read_size)
            : base(base), index(index), scale(scale), displacement(displacement), read_size(read_size)
        {
        }

        mem_op(const reg base, const reg index, const uint64_t scale, const int64_t displacement, const reg_size reg_read_size)
            : base(base), index(index), scale(scale), displacement(displacement)
        {
            read_size = reg_size_to_b(reg_read_size);
        }

        reg base;
        reg index;
        uint64_t scale;
        int64_t displacement;

        uint16_t read_size;
    };

    struct reg_op final
    {
        explicit reg_op(const reg reg)
            : reg(reg)
        {
        }

        reg reg;
    };

    struct imm_op final
    {
        explicit imm_op(const uint64_t value, const bool relative = false)
            : value(value), relative(relative)
        {
        }

        explicit imm_op(const reg_size size)
            : relative(false)
        {
            value = reg_size_to_b(size);
        }

        uint64_t value;
        bool relative;
    };

    struct imm_label_operand final
    {
        explicit imm_label_operand(const asmb::code_label_ptr& value, const bool relative = false, const bool negative = false)
            : code_label(value), relative(relative), negative(negative)
        {
        }

        asmb::code_label_ptr get_dependent() const
        {
            return code_label;
        }

        asmb::code_label_ptr code_label;
        bool relative;

        bool negative;
    };

    struct inst_req
    {
        mnemonic mnemonic;
        std::vector<std::variant<mem_op, reg_op, imm_op, imm_label_operand>> operands;

        std::vector<asmb::code_label_ptr> get_dependents() const
        {
            std::vector<asmb::code_label_ptr> dependents;
            for (auto& op : operands)
            {
                std::visit([&](auto&& inst)
                {
                    using T = std::decay_t<decltype(inst)>;
                    if constexpr (std::is_same_v<T, imm_label_operand>)
                    {
                        const imm_label_operand& label_op = inst;
                        dependents.push_back(label_op.get_dependent());
                    }
                }, op);
            }

            return dependents;
        }

        bool get_rva_dependent() const
        {
            bool is_dependent = false;
            for (auto& op : operands)
            {
                std::visit([&](auto&& inst)
                {
                    using T = std::decay_t<decltype(inst)>;
                    if constexpr (std::is_same_v<T, imm_label_operand>)
                    {
                        const imm_label_operand& label_op = inst;
                        is_dependent |= label_op.relative;
                    }
                    else if constexpr (std::is_same_v<T, imm_op>)
                    {
                        const imm_op& imm_op = inst;
                        is_dependent |= imm_op.relative;
                    }
                }, op);

                if (is_dependent)
                    break;
            }

            return is_dependent;
        }

        enc::req build(uint64_t rva) const
        {
            enc::req req = { };
            req.mnemonic = static_cast<zyids_mnemonic>(mnemonic);

            // call encode_op for each operand
            for (const auto& operand_variant : operands)
            {
                std::visit([&](auto&& operand)
                {
                    encode_op(req, operand, rva);
                }, operand_variant);
            }

            return req;
        }

    private:
        static void encode_op(enc::req& req, const mem_op& mem, uint64_t)
        {
            const auto op_index = req.operand_count;

            req.operands[op_index].type = ZYDIS_OPERAND_TYPE_MEMORY;
            req.operands[op_index].mem.base = static_cast<zydis_register>(mem.base);
            req.operands[op_index].mem.index = static_cast<zydis_register>(mem.index);
            req.operands[op_index].mem.displacement = mem.displacement;
            req.operands[op_index].mem.scale = mem.scale;
            req.operands[op_index].mem.size = mem.read_size;
            req.operand_count++;
        }

        static void encode_op(enc::req& req, const reg_op& reg, uint64_t)
        {
            const auto op_index = req.operand_count;

            req.operands[op_index].type = ZYDIS_OPERAND_TYPE_REGISTER;
            req.operands[op_index].reg.value = static_cast<zydis_register>(reg.reg);
            req.operand_count++;
        }

        static void encode_op(enc::req& req, const imm_op& imm, const uint64_t current_rva)
        {
            const auto op_index = req.operand_count;

            req.operands[op_index].type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
            req.operands[op_index].imm.u = imm.value;
            if (imm.relative)
                req.operands[op_index].imm.u -= current_rva;
            req.operand_count++;
        }

        static void encode_op(enc::req& req, const imm_label_operand& imm, const uint64_t current_rva)
        {
            const auto op_index = req.operand_count;

            req.operands[op_index].type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
            if (imm.negative)
                req.operands[op_index].imm.u = -imm.code_label->get_address();
            else
                req.operands[op_index].imm.u = imm.code_label->get_address();

            if (imm.relative)
                req.operands[op_index].imm.u -= current_rva;

            req.operand_count++;
        }
    };

    class encode_builder
    {
    public:
        template <typename... Args>
        encode_builder& make(const mnemonic mnemonic, Args&&... args)
        {
            inst_req instruction = {};
            instruction.mnemonic = mnemonic;

            // Helper lambda to convert each argument to a specific variant type
            auto make_variant = [](auto&& arg) -> std::variant<mem_op, reg_op, imm_op, imm_label_operand> {
                using ArgType = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<ArgType, mem_op>)
                    return (std::in_place_type<mem_op>, std::forward<ArgType>(arg));
                else if constexpr (std::is_same_v<ArgType, reg_op>)
                    return (std::in_place_type<reg_op>, std::forward<ArgType>(arg));
                else if constexpr (std::is_same_v<ArgType, imm_op>)
                    return (std::in_place_type<imm_op>, std::forward<ArgType>(arg));
                else if constexpr (std::is_same_v<ArgType, imm_label_operand>)
                    return (std::in_place_type<imm_label_operand>, std::forward<ArgType>(arg));
            };

            // Expand and push each argument as a variant into the operands vector
            (instruction.operands.push_back(make_variant(std::forward<Args>(args))), ...);

            instruction_list.push(instruction);
            return *this;
        }

        encode_builder& label(const asmb::code_label_ptr& ptr)
        {
            instruction_list.push(ptr);
            return *this;
        }

        encode_builder& transfer_from(const encode_builder& from)
        {
            while (!from.instruction_list.empty())
            {
                instruction_list.push(from.instruction_list.front());
                instruction_list.pop();
            }

            return *this;
        }

        using inst_req_label_v = std::variant<inst_req, asmb::code_label_ptr>;
        std::queue<inst_req_label_v> instruction_list;
    };

    using encode_builder_ptr = std::shared_ptr<encode_builder>;
}
