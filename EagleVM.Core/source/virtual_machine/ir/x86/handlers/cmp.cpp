#include "eaglevm-core/virtual_machine/ir/x86/handlers/cmp.h"

#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_store.h"

namespace eagle::il::handler
{
    cmp::cmp()
    {
        entries = {
            { { codec::op_none, codec::bit_8 }, { codec::op_none, codec::bit_8 } },
            { { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_16 } },
            { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_32 } },
            { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_64 } },
        };
    }

    il_insts cmp::gen_handler(const codec::reg_class size, uint8_t operands)
    {
        const il_size target_size = static_cast<il_size>(get_reg_size(size));
        const reg_vm vtemp = get_bit_version(reg_vm::vtemp, target_size);
        const reg_vm vtemp2 = get_bit_version(reg_vm::vtemp2, target_size);

        return {
            std::make_shared<cmd_vm_pop>(vtemp, target_size),
            std::make_shared<cmd_vm_pop>(vtemp2, target_size),
            std::make_shared<cmd_x86_dynamic>(codec::m_cmp, vtemp, vtemp2),
            std::make_shared<cmd_vm_push>(vtemp, target_size)
        };
    }
}

namespace eagle::il::lifter
{
    void cmp::finalize_translate_to_virtual()
    {
        block->add_command(std::make_shared<cmd_rflags_load>());
        base_x86_lifter::finalize_translate_to_virtual();
        block->add_command(std::make_shared<cmd_rflags_store>());
    }

    translate_status cmp::encode_operand(codec::dec::op_imm op_imm, uint8_t idx)
    {
        il_size target_size = get_op_width();
        block->add_command(std::make_shared<cmd_vm_push>(op_imm.value.u, target_size));

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
