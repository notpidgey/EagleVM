#include "eaglevm-core/virtual_machine/ir/x86/handlers/cmp.h"

#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_store.h"

namespace eagle::ir::handler
{
    cmp::cmp()
    {
        valid_operands = {
            { { { codec::op_none, codec::bit_8 }, { codec::op_none, codec::bit_8 } }, "cmp 8,8" },
            { { { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_16 } }, "cmp 16,16" },
            { { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_32 } }, "cmp 32,32" },
            { { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_64 } }, "cmp 64,64" },

            // sign extended handlers
            { { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_32 } }, "cmp 64,64" },

            // non sign extended
            { { { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_8 } }, "cmp 16,16" },
            { { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_8 } }, "cmp 32,32" },
            { { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_8 } }, "cmp 64,64" },
        };

        build_options = {
            { { ir_size::bit_8, ir_size::bit_8 }, "cmp 8,8" },
            { { ir_size::bit_16, ir_size::bit_16 }, "cmp 16,16" },
            { { ir_size::bit_32, ir_size::bit_32 }, "cmp 32,32" },
            { { ir_size::bit_64, ir_size::bit_64 }, "cmp 64,64" },
        };
    }

    ir_insts cmp::gen_handler(handler_sig signature)
    {
        VM_ASSERT(signature.size() == 2, "invalid signature. must contain 2 operands");
        VM_ASSERT(signature[0] == signature[1], "invalid signature. must contain same sized parameters");

        ir_size target_size = signature.front();

        const discrete_store_ptr vtemp = discrete_store::create(target_size);
        const discrete_store_ptr vtemp2 = discrete_store::create(target_size);

        return {
            std::make_shared<cmd_pop>(vtemp, target_size),
            std::make_shared<cmd_pop>(vtemp2, target_size),
            std::make_shared<cmd_x86_dynamic>(codec::m_cmp, vtemp2, vtemp)
        };
    }
}

namespace eagle::ir::lifter
{
    void cmp::finalize_translate_to_virtual()
    {
        block->add_command(std::make_shared<cmd_rflags_load>());
        base_x86_translator::finalize_translate_to_virtual();
        block->add_command(std::make_shared<cmd_rflags_store>());
    }

    translate_status cmp::encode_operand(codec::dec::op_imm op_imm, uint8_t idx)
    {
        codec::dec::operand second_op = operands[1];
        codec::dec::operand first_op = operands[0];

        ir_size imm_size = static_cast<ir_size>(second_op.size);
        ir_size imm_size_target = static_cast<ir_size>(first_op.size);

        const bool is_register = first_op.type == ZYDIS_OPERAND_TYPE_REGISTER;
        const bool is_bit64 = codec::get_reg_size(first_op.reg.value) == codec::bit_64;

        if (imm_size == ir_size::bit_32 && is_register && is_bit64)
        {
            // we need to sign extend the IMM value
            // this only happens in two cases
            // 1. REX.W + 3D id	CMP RAX, imm32
            // 2. REX.W + 81 /7 id	CMP r/m64, imm32

            block->add_command(std::make_shared<cmd_push>(op_imm.value.u, imm_size));
            block->add_command(std::make_shared<cmd_sx>(ir_size::bit_64, ir_size::bit_32));

            stack_displacement += static_cast<uint16_t>(TOB(ir_size::bit_64));
        }
        else
        {
            block->add_command(std::make_shared<cmd_push>(op_imm.value.u, imm_size_target));

            stack_displacement += static_cast<uint16_t>(TOB(imm_size_target));
        }

        return translate_status::success;
    }
}
