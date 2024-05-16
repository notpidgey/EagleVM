#include "eaglevm-core/virtual_machine/ir/x86/handlers/movsx.h"

namespace eagle::ir::handler
{
    movsx::movsx()
    {
        entries = {
            { { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_8 } },
            { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_8 } },
            { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_8 } },

            { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_16 } },
            { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_16 } },
        };
    }

    ir_insts movsx::gen_handler(codec::reg_class size, uint8_t operands)
    {
    }
}

namespace eagle::ir::lifter
{
    translate_status movsx::encode_operand(codec::dec::op_mem op_mem, const uint8_t idx)
    {
        if (op_mem.base == ZYDIS_REGISTER_RIP)
        {
            auto [target, _] = codec::calc_relative_rva(inst, operands, orig_rva, idx);
            block->add_command(std::make_shared<cmd_push>(target, ir_size::bit_64, true));

            return translate_status::success;
        }

        // [base + index * scale + disp]
        // 1. begin with loading the base register
        // mov VTEMP, imm
        // jmp VM_LOAD_REG
        if (op_mem.base == ZYDIS_REGISTER_RSP)
        {
            block->add_command(std::make_shared<cmd_push>(reg_vm::vsp, ir_size::bit_64));
            if (stack_displacement)
            {
                block->add_command(std::make_shared<cmd_push>(stack_displacement, ir_size::bit_64));
                block->add_command(std::make_shared<cmd_handler_call>(call_type::inst_handler,
                    codec::m_add, 2, codec::reg_class::gpr_64));
            }
        }
        else
        {
            block->add_command(std::make_shared<cmd_context_load>(codec::reg(op_mem.base), ir_size::bit_64));
        }

        //2. load the index register and multiply by scale
        //mov VTEMP, imm    ;
        //jmp VM_LOAD_REG   ; load value of INDEX reg to the top of the VSTACK
        if (op_mem.index != ZYDIS_REGISTER_NONE)
        {
            block->add_command(std::make_shared<cmd_context_load>(codec::reg(op_mem.index), ir_size::bit_64));
        }

        if (op_mem.scale != 0)
        {
            //mov VTEMP, imm    ;
            //jmp VM_PUSH       ; load value of SCALE to the top of the VSTACK
            //jmp VM_MUL        ; multiply INDEX * SCALE
            block->add_command(std::make_shared<cmd_push>(op_mem.scale, ir_size::bit_64));
            block->add_command(std::make_shared<cmd_handler_call>(call_type::inst_handler,
                codec::m_imul, 2, codec::reg_class::gpr_64));
        }

        if (op_mem.index != ZYDIS_REGISTER_NONE)
        {
            block->add_command(std::make_shared<cmd_handler_call>(call_type::inst_handler,
                codec::m_add, 2, codec::reg_class::gpr_64));
        }

        if (op_mem.disp.has_displacement)
        {
            // 3. load the displacement and add
            // we can do this with some trickery using LEA so we dont modify rflags

            // subtract displacement value
            block->add_command(std::make_shared<cmd_push>(op_mem.disp.value, ir_size::bit_64));
            block->add_command(std::make_shared<cmd_handler_call>(call_type::inst_handler,
                codec::m_sub, 2, codec::reg_class::gpr_64));
        }

        // for memory operands we will only ever need one kind of action
        // there has to be a better and cleaner way of doing this, but i have not thought of it yet
        // for now it will kind of just be an assumption

        if (op_mem.type == ZYDIS_MEMOP_TYPE_AGEN || virtualize_as_address(operands[idx], idx))
        {
            stack_displacement += static_cast<uint16_t>(ir_size::bit_64);
        }
        else
        {
            // by default, this will be dereferenced and we will get the value at the address,
            const ir_size target_size = ir_size(inst.operand_width);
            const reg_vm target_temp = get_bit_version(reg_vm::vtemp, target_size);

            block->add_command(std::make_shared<cmd_pop>(reg_vm::vtemp, ir_size::bit_64));
            block->add_command(std::make_shared<cmd_mem_read>(reg_vm::vtemp, ir_size::bit_64));
            block->add_command(std::make_shared<cmd_push>(target_temp, target_size));

            stack_displacement += static_cast<uint16_t>(target_size);
        }

        const ir_size target = get_op_width();
        const ir_size size = ir_size(operands[idx].size);
        block->add_command(std::make_shared<cmd_sx>(target, size));

        return translate_status::success;
    }
}
