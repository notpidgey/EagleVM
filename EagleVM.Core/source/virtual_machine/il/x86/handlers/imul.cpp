#include "eaglevm-core/virtual_machine/il/x86/handlers/imul.h"

#include "eaglevm-core/virtual_machine/il/commands/cmd_rflags_load.h"
#include "eaglevm-core/virtual_machine/il/commands/cmd_rflags_store.h"

namespace eagle::il::handler
{
    imul::imul()
    {
        entries = {
            { codec::gpr_64, 2 },
            { codec::gpr_32, 2 },
            { codec::gpr_16, 2 },

            // its 3 operands but we handle in finalize_translate_to_virtual
            { codec::gpr_64, 3 },
            { codec::gpr_32, 3 },
            { codec::gpr_16, 3 },
        };
    }

    il_insts imul::gen_handler(codec::reg_class size, uint8_t operands)
    {
        // IMUL r16, r/m16, imm8    word register := r/m16 ∗ sign-extended immediate byte.
        // IMUL r32, r/m32, imm8    doubleword register := r/m32 ∗ sign-extended immediate byte.
        // IMUL r64, r/m64, imm8    Quadword register := r/m64 ∗ sign-extended immediate byte.
    }
}

void eagle::il::lifter::imul::finalize_translate_to_virtual()
{
    if (inst.operand_count_visible == 1)
    {
        // use the same operand twice
        translate_status status = translate_status::unsupported;
        switch (const codec::dec::operand& operand = operands[0]; operand.type)
        {
            case ZYDIS_OPERAND_TYPE_REGISTER:
                status = encode_operand(operand.reg, 0);
                break;
            case ZYDIS_OPERAND_TYPE_MEMORY:
                status = encode_operand(operand.mem, 0);
                break;
        }

        assert(status == translate_status::success, "failed to virtualized operand");
    }

    {
        block->add_command(std::make_shared<cmd_rflags_load>());
        base_x86_lifter::finalize_translate_to_virtual();
        block->add_command(std::make_shared<cmd_rflags_store>());
    }

    switch (inst.operand_count_visible)
    {
        case 1:
        {
            // we do not support yet
            break;
        }
        case 2:
        {
            // the product is at the top of the stack
            // we can save to the destination register by specifying the displacement
            // and then calling store reg
            block->add_command(std::make_shared<cmd_context_store>(codec::reg(operands[0].reg.value)));
            break;
        }
        case 3:
        {
            // only these cases:
            // IMUL r16, r/m16, imm8
            // IMUL r32, r/m32, imm8
            // IMUL r64, r/m64, imm8
            // IMUL r16, r/m16, imm16
            // IMUL r32, r/m32, imm32
            // IMUL r64, r/m64, imm32

            // we want to move op2 -> op1
            // op1 wil always be a reg
            codec::reg_class size = codec::get_reg_class(operands[0].reg.value);
            block->add_command(std::make_shared<cmd_handler_call>(call_type::inst_handler, codec::m_mov, 2, size));
            break;
        }
    }
}
