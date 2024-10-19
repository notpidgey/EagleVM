#include "eaglevm-core/virtual_machine/ir/x86/handlers/dec.h"
#include "eaglevm-core/virtual_machine/ir/x86/handlers/util/flags.h"

#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_rflags_store.h"

namespace eagle::ir::handler
{
    dec::dec()
    {
        // todo: make vector of supported signatures
        // todo: make vector of handlers to generate
        valid_operands = {
            { { { codec::op_none, codec::bit_8 } }, "dec 8" },
            { { { codec::op_none, codec::bit_16 } }, "dec 16" },
            { { { codec::op_none, codec::bit_32 } }, "dec 32" },
            { { { codec::op_none, codec::bit_64 } }, "dec 64" },
        };

        build_options = {
            { { ir_size::bit_8 }, "dec 8" },
            { { ir_size::bit_16 }, "dec 16" },
            { { ir_size::bit_32 }, "dec 32" },
            { { ir_size::bit_64 }, "dec 64" },
        };
    }

    ir_insts dec::gen_handler(handler_sig signature)
    {
        VM_ASSERT(signature.size() == 1, "invalid signature. must contain 1 operand");
        ir_size target_size = signature.front();

        const discrete_store_ptr p_one = discrete_store::create(target_size);
        const discrete_store_ptr result = discrete_store::create(target_size);

        const discrete_store_ptr flags_result = discrete_store::create(ir_size::bit_64);

        constexpr auto affected_flags = ZYDIS_CPUFLAG_OF | ZYDIS_CPUFLAG_SF | ZYDIS_CPUFLAG_ZF | ZYDIS_CPUFLAG_AF | ZYDIS_CPUFLAG_PF;
        ir_insts insts = {
            std::make_shared<cmd_pop>(p_one, target_size),
            make_dyn(codec::m_mov, encoder::reg(result), encoder::reg(p_one)),
            make_dyn(codec::m_dec, encoder::reg(result)),
            std::make_shared<cmd_push>(result, target_size),

            // The CF flag is not affected. The OF, SF, ZF, AF, and PF flags are set according to the result.
            std::make_shared<cmd_context_rflags_load>(),
            std::make_shared<cmd_push>(~affected_flags, ir_size::bit_64),
            std::make_shared<cmd_and>(ir_size::bit_64),
        };

        insts.append_range(util::calculate_sf(target_size, flags_result, result));
        insts.append_range(util::calculate_zf(target_size, flags_result, result));
        insts.append_range(util::calculate_pf(target_size, flags_result, result));

        insts.append_range(compute_of(target_size, result, p_one, flags_result));
        insts.append_range(compute_af(target_size, result, p_one, flags_result));

        return insts;
    }

    ir_insts dec::compute_of(ir_size size, const discrete_store_ptr& result, const discrete_store_ptr& value, const discrete_store_ptr& flags)
    {
        uint64_t max = 0;
        switch (size)
        {
            case ir_size::bit_64:
                max = UINT64_MAX;
                break;
            case ir_size::bit_32:
                max = UINT32_MAX;
                break;
            case ir_size::bit_16:
                max = UINT16_MAX;
                break;
            case ir_size::bit_8:
                max = UINT8_MAX;
                break;
        }

        return {
            std::make_shared<cmd_push>(value, ir_size::bit_64),

            make_dyn(codec::m_cmp, encoder::reg(value), encoder::imm(max)),
            make_dyn(codec::m_xor, encoder::reg(result), encoder::reg(result)),
            make_dyn(codec::m_cmovz, encoder::reg(result), encoder::imm(ZYDIS_CPUFLAG_AF)),
            make_dyn(codec::m_or, encoder::reg(flags), encoder::reg(result)),

            std::make_shared<cmd_pop>(value, ir_size::bit_64),
        };
    }

    ir_insts dec::compute_af(ir_size size, const discrete_store_ptr& result, const discrete_store_ptr& value, const discrete_store_ptr& flags)
    {
        return {
            make_dyn(codec::m_cmp, encoder::reg(value), encoder::imm(0xF)),
            make_dyn(codec::m_xor, encoder::reg(result), encoder::reg(result)),
            make_dyn(codec::m_cmovz, encoder::reg(result), encoder::imm(ZYDIS_CPUFLAG_AF)),
            make_dyn(codec::m_or, encoder::reg(flags), encoder::reg(result)),
        };
    }
}

namespace eagle::ir::lifter
{
    translate_mem_result dec::translate_mem_action(const codec::dec::op_mem& op_mem, uint8_t idx) { return translate_mem_result::both; }

    void dec::finalize_translate_to_virtual(x86_cpu_flag flags)
    {
        base_x86_translator::finalize_translate_to_virtual(flags);

        codec::dec::operand first_op = operands[0];
        if (first_op.type == ZYDIS_OPERAND_TYPE_REGISTER)
        {
            codec::reg reg = static_cast<codec::reg>(first_op.reg.value);
            block->push_back(std::make_shared<cmd_context_store>(reg));
        }
        else if (first_op.type == ZYDIS_OPERAND_TYPE_MEMORY)
        {
            ir_size value_size = static_cast<ir_size>(first_op.size);
            block->push_back(std::make_shared<cmd_mem_write>(value_size, value_size));
        }
    }
}
