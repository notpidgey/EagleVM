#include "eaglevm-core/virtual_machine/ir/x86/handlers/shl.h"
#include "eaglevm-core/virtual_machine/ir/x86/handlers/util/flags.h"

#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_rflags_store.h"

namespace eagle::ir::handler
{
    shl::shl()
    {
        valid_operands = {
            { { { codec::op_none, codec::bit_8 }, { codec::op_none, codec::bit_8 } }, "shl 8,8" },
            { { { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_16 } }, "shl 16,16" },
            { { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_32 } }, "shl 32,32" },
            { { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_64 } }, "shl 64,64" },

            // sign extended handlers
            { { { codec::op_none, codec::bit_16 }, { codec::op_imm, codec::bit_8 } }, "shl 16,16" },
            { { { codec::op_none, codec::bit_32 }, { codec::op_imm, codec::bit_8 } }, "shl 32,32" },
            { { { codec::op_none, codec::bit_64 }, { codec::op_imm, codec::bit_8 } }, "shl 64,64" },
            { { { codec::op_none, codec::bit_64 }, { codec::op_imm, codec::bit_32 } }, "shl 64,64" },
        };

        build_options = {
            { { ir_size::bit_8, ir_size::bit_8 }, "shl 8,8" },
            { { ir_size::bit_16, ir_size::bit_16 }, "shl 16,16" },
            { { ir_size::bit_32, ir_size::bit_32 }, "shl 32,32" },
            { { ir_size::bit_64, ir_size::bit_64 }, "shl 64,64" },
        };
    }

    ir_insts shl::gen_handler(handler_sig signature)
    {
        VM_ASSERT(signature.size() == 2, "invalid signature. must contain 2 operands");
        VM_ASSERT(signature[0] == signature[1], "invalid signature. must contain same sized parameters");

        ir_size target_size = signature.front();

        // the way this is done is far slower than it used to be
        // however because of the way this IL is written, there is far more room to expand how the virtual context is stored
        // in shlition, it gives room for mapping x86 context into random places as well

        const discrete_store_ptr shift_count = discrete_store::create(target_size);
        const discrete_store_ptr shift_value = discrete_store::create(target_size);
        const discrete_store_ptr shift_result = discrete_store::create(target_size);

        const discrete_store_ptr flags_result = discrete_store::create(ir_size::bit_64);

        // todo: some kind of virtual machine implementation where it could potentially try to optimize a pop and use of the register in the next
        // instruction using stack dereference
        ir_insts insts = {
            std::make_shared<cmd_pop>(shift_count, target_size),
            std::make_shared<cmd_pop>(shift_value, target_size),
            make_dyn(codec::m_mov, encoder::reg(shift_result), encoder::reg(shift_value)),
            make_dyn(codec::m_shl, encoder::reg(shift_result), encoder::reg(shift_count)),
            std::make_shared<cmd_push>(shift_value, target_size),

            /*
                The CF flag contains the value of the last bit shifted out of the destination operand; it is undefined for SHL and SHR instructions
               where the count is greater than or equal to the size (in bits) of the destination operand. The OF flag is affected only for 1-bit
               shifts (see “Description” above); otherwise, it is undefined. The SF, ZF, and PF flags are set according to the result. If the count is
               0, the flags are not affected. For a non-zero count, the AF flag is undefined.
            */
            std::make_shared<cmd_context_rflags_load>(),
            std::make_shared<cmd_pop>(flags_result, ir_size::bit_64),

            // CF, OF, SF, ZF, PF are all set
            make_dyn(codec::m_and, encoder::reg(flags_result),
                     encoder::imm(~(ZYDIS_CPUFLAG_CF | ZYDIS_CPUFLAG_OF | ZYDIS_CPUFLAG_SF | ZYDIS_CPUFLAG_ZF | ZYDIS_CPUFLAG_PF))),
        };

        insts.append_range(compute_cf(target_size, shift_result, shift_value, shift_count, flags_result));
        insts.append_range(compute_of(target_size, shift_result, shift_value, shift_count, flags_result));

        // The SF, ZF, and PF flags are set according to the result.
        insts.append_range(util::calculate_sf(target_size, flags_result, shift_result));
        insts.append_range(util::calculate_zf(target_size, flags_result, shift_result));
        insts.append_range(util::calculate_pf(target_size, flags_result, shift_result));

        return insts;
    }

    ir_insts shl::compute_cf(ir_size size, const discrete_store_ptr& shift_result, const discrete_store_ptr& shift_value,
                             const discrete_store_ptr& shift_count, const discrete_store_ptr& flags_result)
    {
        //
        // CF = (value >> (shift_count - 1))
        //

        return {
            std::make_shared<cmd_push>(shift_value, ir_size::bit_64),

            // value >> (shift_count - 1)
            make_dyn(codec::m_dec, encoder::reg(shift_count)),
            make_dyn(codec::m_shr, encoder::reg(shift_value), encoder::reg(shift_count)),
            make_dyn(codec::m_inc, encoder::reg(shift_count)), // restore shift_count

            // CF = (value >> (shift_count - 1))
            make_dyn(codec::m_and, encoder::reg(shift_value), encoder::imm(1)),
            make_dyn(codec::m_shl, encoder::reg(shift_value), encoder::imm(util::flag_index(ZYDIS_CPUFLAG_CF))),
            make_dyn(codec::m_or, encoder::reg(flags_result), encoder::reg(shift_value)),

            std::make_shared<cmd_pop>(shift_value, ir_size::bit_64),
        };
    }

    ir_insts shl::compute_of(ir_size size, const discrete_store_ptr& shift_result, const discrete_store_ptr& shift_value,
                             const discrete_store_ptr& shift_count, const discrete_store_ptr& flags_result)
    {
        //
        // OF = MSB(tempDEST) XOR CF
        //

        return {
            std::make_shared<cmd_push>(shift_value, size),
            std::make_shared<cmd_push>(shift_count, size),

            make_dyn(codec::m_shr, encoder::reg(shift_value), encoder::imm(static_cast<uint64_t>(size) - 1)),
            make_dyn(codec::m_and, encoder::reg(shift_value), encoder::imm(1)),

            make_dyn(codec::m_mov, encoder::reg(shift_count), encoder::reg(flags_result)),
            make_dyn(codec::m_and, encoder::reg(shift_count), encoder::imm(ZYDIS_CPUFLAG_CF)),
            make_dyn(codec::m_shr, encoder::reg(shift_count), encoder::imm(util::flag_index(ZYDIS_CPUFLAG_CF))),
            make_dyn(codec::m_xor, encoder::reg(shift_value), encoder::reg(shift_count)),

            // move most significant bit to the OF position and set it in "flags_result"
            make_dyn(codec::m_shl, encoder::reg(shift_value), encoder::imm((util::flag_index(ZYDIS_CPUFLAG_OF)))),
            make_dyn(codec::m_or, encoder::reg(flags_result), encoder::reg(shift_value)),

            std::make_shared<cmd_pop>(shift_count, size),
            std::make_shared<cmd_pop>(shift_value, size),
        };
    }
}

namespace eagle::ir::lifter
{
    translate_mem_result shl::translate_mem_action(const codec::dec::op_mem& op_mem, uint8_t idx)
    {
        return idx == 0 ? translate_mem_result::both : base_x86_translator::translate_mem_action(op_mem, idx);
    }

    translate_status shl::encode_operand(codec::dec::op_imm op_imm, uint8_t)
    {
        codec::dec::operand second_op = operands[1];
        codec::dec::operand first_op = operands[0];

        ir_size imm_size = static_cast<ir_size>(second_op.size);
        ir_size imm_size_target = static_cast<ir_size>(first_op.size);

        block->push_back(std::make_shared<cmd_push>(op_imm.value.u, imm_size));
        if (imm_size != imm_size_target)
            block->push_back(std::make_shared<cmd_sx>(imm_size_target, imm_size));

        return translate_status::success;
    }

    void shl::finalize_translate_to_virtual(x86_cpu_flag flags)
    {
        base_x86_translator::finalize_translate_to_virtual(flags);

        codec::dec::operand first_op = operands[0];
        if (first_op.type == ZYDIS_OPERAND_TYPE_REGISTER)
        {
            // register
            codec::reg reg = static_cast<codec::reg>(first_op.reg.value);
            if (static_cast<ir_size>(first_op.size) == ir_size::bit_32)
                reg = codec::get_bit_version(first_op.reg.value, codec::gpr_64);

            block->push_back(std::make_shared<cmd_context_store>(reg, static_cast<codec::reg_size>(first_op.size)));
        }
        else if (first_op.type == ZYDIS_OPERAND_TYPE_MEMORY)
        {
            ir_size value_size = static_cast<ir_size>(first_op.size);
            block->push_back(std::make_shared<cmd_mem_write>(value_size, value_size));
        }
    }
}
