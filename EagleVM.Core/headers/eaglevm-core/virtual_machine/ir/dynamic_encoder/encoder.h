#pragma once
#include <memory>
#include <vector>
#include <variant>

#include "eaglevm-core/virtual_machine/ir/models/ir_discrete_reg.h"
#include "eaglevm-core/virtual_machine/ir/models/ir_store.h"
#include "eaglevm-core/codec/zydis_helper.h"

namespace eagle::ir::encoder
{
    using val_variant = std::variant<std::monostate, reg_vm, discrete_store_ptr, uint64_t>;

    class operand
    {
    public:
        virtual ~operand() = default;

        template <typename FunResolve>
        virtual void build(codec::enc::req& req, FunResolve resolver) = 0;
    };

    // Operand for memory operations
    class operand_mem final : public operand
    {
    public:
        // Constructors for different use cases
        operand_mem(const val_variant& base, const ir_size size)
            : base_(base), size_(size)
        {
            VM_ASSERT(size != ir_size::none, "Size must not be empty");
        }

        operand_mem(const val_variant& base, const val_variant& index, const uint8_t scale, const ir_size size)
            : base_(base), index_(index), scale_(scale), size_(size)
        {
            VM_ASSERT(size != ir_size::none, "Size must not be empty");
        }

        template <typename FunResolve>
        void build(codec::enc::req& req, FunResolve resolver) override
        {
            codec::enc::op_mem mem{ };
            mem.base = resolver(base_);
            mem.index = resolver(index_);
            mem.scale = scale_;
            mem.size = static_cast<uint16_t>(size_);

            codec::add_op(req, mem);
        }

    private:
        val_variant base_;
        val_variant index_ = std::monostate{ };
        uint8_t scale_ = 1;
        ir_size size_;
    };

    // Operand for register operations
    class operand_reg final : public operand
    {
    public:
        explicit operand_reg(const val_variant& reg)
            : reg_(reg)
        {
        }

        template <typename FunResolve>
        void build(codec::enc::req& req, FunResolve resolver) override
        {
            codec::enc::op_reg op_reg{ };
            op_reg.value = resolver(reg_);

            codec::add_op(req, op_reg);
        }

    private:
        val_variant reg_;
    };

    // Operand for immediate values
    class operand_imm final : public operand
    {
    public:
        explicit operand_imm(const uint64_t imm)
            : imm_(imm)
        {
        }

        template <typename FunResolve>
        void build(codec::enc::req& req, FunResolve) override
        {
            codec::enc::op_imm im{ };
            im.s = imm_;

            codec::add_op(req, im);
        }

    private:
        uint64_t imm_;
    };

    // Encoder class to handle instruction encoding
    class encoder
    {
    public:
        template <typename... Operands>
        explicit encoder(const codec::mnemonic mnemonic, Operands... ops)
            : mnemonic_(mnemonic), operands_{ ops... }
        {
        }

        template <typename FunResolve>
        codec::enc::req create(FunResolve resolver)
        {
            auto req = encode(mnemonic_);
            for (const auto& op : operands_)
                op->build(req, resolver);

            return req;
        }

    private:
        codec::mnemonic mnemonic_;
        std::vector<std::shared_ptr<operand>> operands_;
    };

    inline std::shared_ptr<operand> reg(const val_variant& reg)
    {
        return std::make_shared<operand_reg>(reg);
    }

    inline std::shared_ptr<operand> imm(uint64_t imm_value)
    {
        return std::make_shared<operand_imm>(imm_value);
    }

    inline std::shared_ptr<operand> mem(const val_variant& base, ir_size size)
    {
        return std::make_shared<operand_mem>(base, size);
    }

    inline std::shared_ptr<operand> mem(const val_variant& base, const val_variant& index, uint8_t scale, ir_size size)
    {
        return std::make_shared<operand_mem>(base, index, scale, size);
    }
}
