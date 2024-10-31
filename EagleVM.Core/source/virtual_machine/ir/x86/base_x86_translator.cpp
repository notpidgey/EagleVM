#include "eaglevm-core/virtual_machine/ir/x86/base_x86_translator.h"

#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_handler_call.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_mem_read.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_pop.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_push.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_rflags_store.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_logic.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_x86_dynamic.h"
#include "eaglevm-core/virtual_machine/ir/models/ir_size.h"

namespace eagle::ir::lifter
{
    base_x86_translator::base_x86_translator(const std::shared_ptr<class ir_translator>& translator, codec::dec::inst_info decode,
        const uint64_t rva) :
        translator(translator), block(std::make_shared<block_ir>(vm_block)), orig_rva(rva), inst(decode.instruction)
    {
        inst = decode.instruction;
        std::ranges::copy(decode.operands, std::begin(operands));
    }

    bool base_x86_translator::translate_to_il(uint64_t original_rva, x86_cpu_flag flags)
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

        finalize_translate_to_virtual(flags);
        return true;
    }

    block_ptr base_x86_translator::get_block() { return block; }

    void base_x86_translator::finalize_translate_to_virtual(const x86_cpu_flag flags)
    {
        x86_operand_sig operand_sig = { };
        for (uint8_t i = 0; i < inst.operand_count_visible; i++)
        {
            operand_sig.emplace_back(static_cast<codec::op_type>(operands[i].type), static_cast<codec::reg_size>(operands[i].size));
        }

        // places rflags on top of the stack
        block->push_back(std::make_shared<cmd_handler_call>(static_cast<codec::mnemonic>(inst.mnemonic), operand_sig));

        // zero change
        if (flags != 0)
            block->push_back(std::make_shared<cmd_context_rflags_store>(flags));

        // pops rflags
        block->push_back(std::make_shared<cmd_pop>(ir_size::bit_64));
    }

    translate_status base_x86_translator::encode_operand(codec::dec::op_reg op_reg, uint8_t idx)
    {
        block->push_back(std::make_shared<cmd_context_load>(static_cast<codec::reg>(op_reg.value)));
        return translate_status::success;
    }

    translate_status base_x86_translator::encode_operand(codec::dec::op_mem op_mem, const uint8_t idx)
    {
        if (op_mem.type != ZYDIS_MEMOP_TYPE_MEM && op_mem.type != ZYDIS_MEMOP_TYPE_AGEN)
            return translate_status::unsupported;

        // [base + index * scale + disp]
        // 1. loading the base register
        if (op_mem.base == ZYDIS_REGISTER_RIP)
        {
            auto [target, _] = codec::calc_relative_rva(inst, operands, orig_rva, idx);
            block->push_back(std::make_shared<cmd_push>(target, ir_size::bit_64));

            goto HANDLE_MEM_ACTION;
        }

        if (op_mem.base == ZYDIS_REGISTER_RSP)
        {
            block->push_back(std::make_shared<cmd_push>(reg_vm::vsp, ir_size::bit_64));
            if (stack_displacement)
            {
                block->push_back(std::make_shared<cmd_push>(stack_displacement, ir_size::bit_64));
                block->push_back(std::make_shared<cmd_handler_call>(codec::m_add, handler_sig{ ir_size::bit_64, ir_size::bit_64 }));
            }
        }
        else if (op_mem.base != ZYDIS_REGISTER_NONE)
        {
            block->push_back(std::make_shared<cmd_context_load>(static_cast<codec::reg>(op_mem.base)));
        }
        else
        {
            // selector
            block->push_back(std::make_shared<cmd_context_load>(static_cast<codec::reg>(op_mem.segment)));
        }

        // 2. load the index register and multiply by scale
        // mov VTEMP, imm    ;
        // jmp VM_LOAD_REG   ; load value of INDEX reg to the top of the VSTACK
        if (op_mem.index != ZYDIS_REGISTER_NONE)
        {
            block->push_back(std::make_shared<cmd_context_load>(static_cast<codec::reg>(op_mem.index)));
        }

        if (op_mem.scale != 0)
        {
            block->push_back(std::make_shared<cmd_push>(op_mem.scale, ir_size::bit_64));
            block->push_back(std::make_shared<cmd_handler_call>(codec::m_imul, handler_sig{ ir_size::bit_64, ir_size::bit_64 }));
        }

        if (op_mem.index != ZYDIS_REGISTER_NONE)
        {
            block->push_back(std::make_shared<cmd_handler_call>(codec::m_add, handler_sig{ ir_size::bit_64, ir_size::bit_64 }));
        }

        if (op_mem.disp.has_displacement)
        {
            // 3. load the displacement and add
            // we can do this with some trickery using LEA so we dont modify rflags

            // subtract displacement value
            block->push_back(std::make_shared<cmd_push>(op_mem.disp.value, ir_size::bit_64));
            block->push_back(std::make_shared<cmd_handler_call>(codec::m_add, handler_sig{ ir_size::bit_64, ir_size::bit_64 }));
        }

        // for memory operands we will only ever need one kind of action
        // there has to be a better and cleaner way of doing this, but i have not thought of it yet
        // for now it will kind of just be an assumption

    HANDLE_MEM_ACTION:
        switch (translate_mem_action(op_mem, idx))
        {
            case translate_mem_result::address:
            {
                stack_displacement += static_cast<uint16_t>(TOB(ir_size::bit_64));
                break;
            }
            case translate_mem_result::value:
            {
                // by default, this will be dereferenced and we will get the value at the address,
                const ir_size target_size = static_cast<ir_size>(inst.operand_width);
                block->push_back(std::make_shared<cmd_mem_read>(target_size));

                stack_displacement += static_cast<uint16_t>(target_size);
                break;
            }
            case translate_mem_result::both:
            {
                // todo: find a better way of doing this
                // for instance adding a pop which actually doesnt remove from the stack and just reads
                // or idfk

                ir_size size = static_cast<ir_size>(operands[idx].size);
                block->push_back({
                    std::make_shared<cmd_dup>(ir_size::bit_64),
                    std::make_shared<cmd_mem_read>(size)
                });

                stack_displacement += static_cast<uint16_t>(TOB(size) + TOB(ir_size::bit_64));
                break;
            }
        }

        return translate_status::success;
    }

    translate_status base_x86_translator::encode_operand(codec::dec::op_ptr op_ptr, uint8_t idx)
    {
        // not a supported operand
        return translate_status::unsupported;
    }

    translate_status base_x86_translator::encode_operand(codec::dec::op_imm op_imm, uint8_t idx)
    {
        const ir_size target_size = static_cast<ir_size>(inst.operand_width);
        block->push_back(std::make_shared<cmd_push>(op_imm.value.u, target_size));

        stack_displacement += static_cast<uint16_t>(target_size);
        return translate_status::success;
    }

    translate_mem_result base_x86_translator::translate_mem_action(const codec::dec::op_mem& op_mem, uint8_t)
    {
        if (op_mem.type == ZYDIS_MEMOP_TYPE_AGEN)
            return translate_mem_result::address;

        return translate_mem_result::value;
    }

    bool base_x86_translator::skip(const uint8_t idx) { return false; }

    ir_size base_x86_translator::get_op_width() const
    {
        uint8_t width = inst.operand_width;
        return static_cast<ir_size>(width);
    }
}
