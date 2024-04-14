#include "eaglevm-core/virtual_machine/il/x86/handlers/movsx.h"

namespace eagle::il::handler
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

    il_insts movsx::gen_handler(codec::reg_class size, uint8_t operands)
    {
    }
}

namespace eagle::il::lifter
{
    translate_status movsx::encode_operand(codec::dec::op_mem op_mem, const uint8_t idx)
    {
        if (op_mem.base == ZYDIS_REGISTER_RIP)
        {
            auto [target, _] = codec::calc_relative_rva(inst, operands, orig_rva, idx);
            block->add_command(std::make_shared<cmd_vm_push>(target, il_size::bit_64, true));

            return translate_status::success;
        }

        // [base + index * scale + disp]
        // 1. begin with loading the base register
        // mov VTEMP, imm
        // jmp VM_LOAD_REG
        if (op_mem.base == ZYDIS_REGISTER_RSP)
        {
            block->add_command(std::make_shared<cmd_vm_push>(reg_vm::vsp, il_size::bit_64));
            if (stack_displacement)
            {
                block->add_command(std::make_shared<cmd_vm_push>(stack_displacement, il_size::bit_64));
                block->add_command(std::make_shared<cmd_handler_call>(call_type::inst_handler,
                    codec::m_add, 2, codec::reg_class::gpr_64));
            }
        }
        else
        {
            block->add_command(std::make_shared<cmd_context_load>(codec::reg(op_mem.base), il_size::bit_64));
        }

        //2. load the index register and multiply by scale
        //mov VTEMP, imm    ;
        //jmp VM_LOAD_REG   ; load value of INDEX reg to the top of the VSTACK
        if (op_mem.index != ZYDIS_REGISTER_NONE)
        {
            block->add_command(std::make_shared<cmd_context_load>(codec::reg(op_mem.index), il_size::bit_64));
        }

        if (op_mem.scale != 0)
        {
            //mov VTEMP, imm    ;
            //jmp VM_PUSH       ; load value of SCALE to the top of the VSTACK
            //jmp VM_MUL        ; multiply INDEX * SCALE
            block->add_command(std::make_shared<cmd_vm_push>(op_mem.scale, il_size::bit_64));
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
            block->add_command(std::make_shared<cmd_vm_push>(op_mem.disp.value, il_size::bit_64));
            block->add_command(std::make_shared<cmd_handler_call>(call_type::inst_handler,
                codec::m_sub, 2, codec::reg_class::gpr_64));
        }

        // for memory operands we will only ever need one kind of action
        // there has to be a better and cleaner way of doing this, but i have not thought of it yet
        // for now it will kind of just be an assumption

        if (op_mem.type == ZYDIS_MEMOP_TYPE_AGEN || virtualize_as_address(operands[idx], idx))
        {
            stack_displacement += static_cast<uint16_t>(il_size::bit_64);
        }
        else
        {
            // by default, this will be dereferenced and we will get the value at the address,
            const il_size target_size = il_size(inst.operand_width);
            const reg_vm target_temp = get_bit_version(reg_vm::vtemp, target_size);

            block->add_command(std::make_shared<cmd_vm_pop>(reg_vm::vtemp, il_size::bit_64));
            block->add_command(std::make_shared<cmd_mem_read>(reg_vm::vtemp, il_size::bit_64));
            block->add_command(std::make_shared<cmd_vm_push>(target_temp, target_size));

            stack_displacement += static_cast<uint16_t>(target_size);
        }

        const il_size target = get_op_width();
        const il_size size = il_size(operands[idx].size);
        block->add_command(std::make_shared<cmd_sx>(target, size));

        return translate_status::success;
    }
}
