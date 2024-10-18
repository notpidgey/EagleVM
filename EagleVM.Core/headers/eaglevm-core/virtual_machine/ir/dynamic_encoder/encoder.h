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
    using val_var_resolver = std::function<codec::reg(val_variant&)>;

    enum class operand_type
    {
        reg,
        imm,
        mem
    };

    class operand
    {
    public:
        virtual ~operand() = default;
        virtual void build(codec::enc::req& req, const val_var_resolver& resolver) = 0;

        operand_type operand_type;
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
            operand_type = operand_type::mem;
        }

        operand_mem(const val_variant& base, const val_variant& index, const uint8_t scale, const ir_size size)
            : base_(base), index_(index), scale_(scale), size_(size)
        {
            VM_ASSERT(size != ir_size::none, "Size must not be empty");
            operand_type = operand_type::mem;
        }

        void build(codec::enc::req& req, const val_var_resolver& resolver) override
        {
            codec::enc::op_mem mem{ };
            mem.base = static_cast<codec::zydis_register>(resolver(base_));
            mem.index = static_cast<codec::zydis_register>(resolver(index_));
            mem.scale = scale_;
            mem.size = static_cast<uint16_t>(size_);

            codec::add_op(req, mem);
        }

        val_variant get_base() { return base_; }
        val_variant get_index() { return index_; }
        uint8_t get_scale() { return scale_; }
        ir_size get_size() { return size_; }

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
            operand_type = operand_type::reg;
        }

        void build(codec::enc::req& req, const val_var_resolver& resolver) override
        {
            codec::enc::op_reg op_reg{ };
            op_reg.value = static_cast<codec::zydis_register>(resolver(reg_));

            codec::add_op(req, op_reg);
        }

        val_variant get_reg()
        {
            return reg_;
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
            operand_type = operand_type::imm;
        }

        void build(codec::enc::req& req, const val_var_resolver&) override
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
        explicit encoder(const codec::mnemonic mnemonic) :
            mnemonic_(mnemonic)
        {

        }

        void add_operand(const std::shared_ptr<operand>& op)
        {
            operands_.push_back(op);
        }

        codec::enc::req create(const val_var_resolver& resolver) const
        {
            auto req = encode(mnemonic_);
            for (const auto& op : operands_)
                op->build(req, resolver);

            return req;
        }

        codec::mnemonic get_mnemonic() const
        {
            return mnemonic_;
        }

        std::vector<std::shared_ptr<operand>> get_operands()
        {
            return operands_;
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
