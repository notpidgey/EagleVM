#include "eaglevm-core/virtual_machine/il/x86/handlers/cmp.h"

namespace eagle::il::handler
{
    cmp::cmp()
    {
        entries = {
            { codec::gpr_64, 2 },
            { codec::gpr_32, 2 },
            { codec::gpr_16, 2 },
            { codec::gpr_8, 2 }
        };
    }

    il_insts cmp::gen_handler(codec::reg_class size, uint8_t operands)
    {
    }
}

namespace eagle::il::lifter
{
    translate_status cmp::encode_operand(codec::dec::op_mem op_mem, uint8_t idx)
    {
        il_size target_size = get_op_width();
        block->add_command(std::make_shared<cmd_vm_push>(op_mem.value.u, target_size));

        const codec::dec::operand first_op = operands[0];
        const bool is_register = first_op.type == ZYDIS_OPERAND_TYPE_REGISTER;
        const bool is_bit64 = codec::get_reg_size(first_op.reg.value) == codec::bit_64;

        if (is_register && is_bit64)
        {
            // we need to sign extend the IMM value
            // this only happens in two cases
            // 1. REX.W + 3D id	CMP RAX, imm32
            // 2. REX.W + 81 /7 id	CMP r/m64, imm32
            block->add_command(std::make_shared<cmd_sx>(il_size::bit_64, il_size::bit_32));
        }

        stack_displacement += static_cast<uint8_t>(target_size);
        return translate_status::success;
    }
}