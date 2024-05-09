#include "eaglevm-core/virtual_machine/ir/x86/base_x86_lifter.h"

#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_push.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_handler_call.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_mem_read.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_pop.h"
#include "eaglevm-core/virtual_machine/ir/models/il_size.h"

namespace eagle::il::lifter
{
    base_x86_lifter::base_x86_lifter(codec::dec::inst_info decode, const uint64_t rva)
        : block(std::make_shared<block_il>(false)), orig_rva(rva), inst(decode.instruction)
    {
        inst = decode.instruction;
        std::ranges::copy(decode.operands, std::begin(operands));
    }

    bool base_x86_lifter::translate_to_il(uint64_t original_rva)
    {
        for (uint8_t i = 0; i < inst.operand_count_visible; i++)
        {
            if (skip(i))
                continue;

            translate_status status = translate_status::unsupported;
            switch (const codec::dec::operand& operand = operands[i]; operand.type)
            {
                case ZYDIS_OPERAND_TYPE_UNUSED:
                    break;
                case ZYDIS_OPERAND_TYPE_REGISTER:
                    status = encode_operand(operand.reg, i);
                    break;
                case ZYDIS_OPERAND_TYPE_MEMORY:
                    status = encode_operand(operand.mem, i);
                    break;
                case ZYDIS_OPERAND_TYPE_POINTER:
                    status = encode_operand(operand.ptr, i);
                    break;
                case ZYDIS_OPERAND_TYPE_IMMEDIATE:
                    status = encode_operand(operand.imm, i);
                    break;
            }

            if (status != translate_status::success)
                return false;
        }

        finalize_translate_to_virtual();
        return true;
    }

    block_il_ptr base_x86_lifter::get_block()
    {
        return block;
    }

    void base_x86_lifter::finalize_translate_to_virtual()
    {
        block->add_command(std::make_shared<cmd_handler_call>(call_type::inst_handler, codec::mnemonic(inst.mnemonic),))
    }

    bool base_x86_lifter::virtualize_as_address(codec::dec::operand operand, const uint8_t idx)
    {
        return idx == 0;
    }

    translate_status base_x86_lifter::encode_operand(codec::dec::op_reg op_reg, uint8_t idx)
    {
        const codec::reg_size size = codec::get_reg_size(op_reg.value);
        block->add_command(std::make_shared<cmd_context_load>(static_cast<codec::reg>(op_reg.value), size));

        return translate_status::success;
    }

    translate_status base_x86_lifter::encode_operand(codec::dec::op_mem op_mem, const uint8_t idx)
    {
        if (op_mem.type != ZYDIS_MEMOP_TYPE_MEM && op_mem.type != ZYDIS_MEMOP_TYPE_AGEN)
            return translate_status::unsupported;

        if (op_mem.base == ZYDIS_REGISTER_RIP)
        {
            auto [target, _] = codec::calc_relative_rva(inst, operands, orig_rva, idx);
            block->add_command(std::make_shared<cmd_vm_push>(target, il_size::bit_64, true));

            return translate_status::success;
        }

        // [base + index * scale + disp]
        // 1. loading the base register
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
            block->add_command(std::make_shared<cmd_context_load>(static_cast<codec::reg>(op_mem.base), il_size::bit_64));
        }

        //2. load the index register and multiply by scale
        //mov VTEMP, imm    ;
        //jmp VM_LOAD_REG   ; load value of INDEX reg to the top of the VSTACK
        if (op_mem.index != ZYDIS_REGISTER_NONE)
        {
            block->add_command(std::make_shared<cmd_context_load>(static_cast<codec::reg>(op_mem.index), il_size::bit_64));
        }

        if (op_mem.scale != 0)
        {
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
            const il_size target_size = static_cast<il_size>(inst.operand_width);
            const reg_vm target_temp = get_bit_version(reg_vm::vtemp, target_size);

            block->add_command(std::make_shared<cmd_vm_pop>(reg_vm::vtemp, il_size::bit_64));
            block->add_command(std::make_shared<cmd_mem_read>(reg_vm::vtemp, il_size::bit_64));
            block->add_command(std::make_shared<cmd_vm_push>(target_temp, target_size));

            stack_displacement += static_cast<uint16_t>(target_size);
        }

        return translate_status::success;
    }

    translate_status base_x86_lifter::encode_operand(codec::dec::op_ptr op_ptr, uint8_t idx)
    {
        // not a supported operand
        return translate_status::unsupported;
    }

    translate_status base_x86_lifter::encode_operand(codec::dec::op_imm op_mem, uint8_t idx)
    {
        const il_size target_size = static_cast<il_size>(inst.operand_width);
        block->add_command(std::make_shared<cmd_vm_push>(op_mem.value.u, target_size));

        stack_displacement += static_cast<uint16_t>(target_size);
        return translate_status::success;
    }

    bool base_x86_lifter::skip(const uint8_t idx)
    {
        return false;
    }

    il_size base_x86_lifter::get_op_width() const
    {
        uint8_t width = inst.operand_width;
        return static_cast<il_size>(width);
    }
}
