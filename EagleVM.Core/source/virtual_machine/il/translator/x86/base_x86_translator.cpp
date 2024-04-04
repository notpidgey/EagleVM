#include "eaglevm-core/virtual_machine/il/translator/x86/base_x86_translator.h"

#include "eaglevm-core/virtual_machine/il/commands/base_command.h"
#include "eaglevm-core/virtual_machine/il/commands/cmd_push.h"
#include "eaglevm-core/virtual_machine/il/commands/cmd_context_load.h"
#include "eaglevm-core/virtual_machine/il/commands/cmd_handler_call.h"
#include "eaglevm-core/virtual_machine/il/commands/cmd_pop.h"

namespace eagle::il::translator
{
    base_x86_translator::base_x86_translator(il_bb_ptr block_ptr, codec::dec::inst_info decode, const uint64_t rva)
        : block(std::move(block_ptr)), orig_rva(rva), inst(decode.instruction)
    {
        inst = decode.instruction;
        std::ranges::copy(decode.operands, std::begin(operands));
    }

    base_x86_translator::base_x86_translator(codec::dec::inst_info decode, const uint64_t rva)
        : block(std::make_shared<il_bb>(false)), orig_rva(rva), inst(decode.instruction)
    {
        inst = decode.instruction;
        std::ranges::copy(decode.operands, std::begin(operands));
    }

    bool base_x86_translator::translate_to_il(uint64_t original_rva)
    {
        for (uint8_t i = 0; i < inst.operand_count_visible; i++)
        {
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

    translate_status base_x86_translator::encode_operand(codec::dec::op_reg op_reg, uint8_t idx)
    {
        // TODO: what about cases where we have RSP as the register?

        // we will always want the address/offset right at the bottom of the stack
        // in the future this might change, but for now it will stay like this

        auto size = codec::get_reg_size(op_reg);
        block->add_command(std::make_shared<cmd_context_load>(op_reg, size));

        return translate_status::success;
    }

    translate_status base_x86_translator::encode_operand(codec::dec::op_mem op_mem, uint8_t idx)
    {
        if (op_mem.type != ZYDIS_MEMOP_TYPE_MEM && op_mem.type != ZYDIS_MEMOP_TYPE_AGEN)
            return translate_status::unsupported;

        //[base + index * scale + disp]

        //1. begin with loading the base register
        //mov VTEMP, imm
        //jmp VM_LOAD_REG
        asmb::code_label* rip_label;
        {
            if (op_mem.base == ZYDIS_REGISTER_RSP)
            {
                block->add_command(std::make_shared<cmd_vm_push>(reg_vm::vsp, stack_disp::bit_64));
                if (stack_displacement)
                {
                    block->add_command(std::make_shared<cmd_vm_push>(stack_displacement, stack_disp::bit_64));
                    block->add_command(std::make_shared<cmd_handler_call>(call_type::inst_handler,
                        codec::ADD, 2, codec::reg_class::gpr_64));
                }
            }
            else if (op_mem.base == ZYDIS_REGISTER_RIP)
            {
                block->add_command(std::make_shared<cmd_context_load>(codec::rip, stack_disp::bit_64));
            }
            else
            {
                block->add_command(std::make_shared<cmd_context_load>(op_mem.base, stack_disp::bit_64));
            }
        }

        //2. load the index register and multiply by scale
        //mov VTEMP, imm    ;
        //jmp VM_LOAD_REG   ; load value of INDEX reg to the top of the VSTACK
        if (op_mem.index != ZYDIS_REGISTER_NONE)
        {
            block->add_command(std::make_shared<cmd_context_load>(op_mem.index, stack_disp::bit_64));
        }

        if (op_mem.scale != 0)
        {
            //mov VTEMP, imm    ;
            //jmp VM_PUSH       ; load value of SCALE to the top of the VSTACK
            //jmp VM_MUL        ; multiply INDEX * SCALE
            block->add_command(std::make_shared<cmd_vm_push>(op_mem.scale, stack_disp::bit_64));
            block->add_command(std::make_shared<cmd_handler_call>(call_type::inst_handler,
                codec::IMUL, 2, codec::reg_class::gpr_64));
        }

        if (op_mem.index != ZYDIS_REGISTER_NONE)
        {
            block->add_command(std::make_shared<cmd_handler_call>(call_type::inst_handler,
                codec::ADD, 2, codec::reg_class::gpr_64));
        }

        if (op_mem.disp.has_displacement)
        {
            // 3. load the displacement and add
            // we can do this with some trickery using LEA so we dont modify rflags

            if (op_mem.base == ZYDIS_REGISTER_RIP)
            {
                // since this is RIP relative we first want to calculate where the original instruction is trying to access
                auto [target, _] = codec::calc_relative_rva(inst, operands, orig_rva, idx);

                // VTEMP = RIP at first operand instruction
                // target = RIP + constant

                call_instruction_handler(container, ZYDIS_MNEMONIC_POP, bit64, 1, true);
                container.add([=](uint64_t)
                {
                    const uint64_t rip = rip_label->get();
                    const uint64_t constant = target - rip;

                    return zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP), ZMEMBD(VTEMP, constant, 8));
                });

                call_instruction_handler(container, ZYDIS_MNEMONIC_PUSH, bit64, 1, true);
            }
            else
            {
                // subtract displacement value
                block->add_command(std::make_shared<cmd_vm_push>(op_mem.disp.value, stack_disp::bit_64));
                block->add_command(std::make_shared<cmd_handler_call>(call_type::inst_handler,
                    codec::SUB, 2, codec::reg_class::gpr_64));
            }
        }

        // for memory operands we will only ever need one kind of action
        // there has to be a better and cleaner way of doing this, but i have not thought of it yet
        // for now it will kind of just be an assumption

        if (op_mem.type == ZYDIS_MEMOP_TYPE_AGEN)
        {
            stack_displacement += static_cast<uint8_t>(stack_disp::bit_64);
        }
        else
        {
            // by default, this will be dereferenced and we will get the value at the address,
            const reg_size target_size = static_cast<reg_size>(inst.operand_width / 8);

            // this means we are working with the second operand
            const reg target_temp = get_bit_version(VTEMP, target_size);

            block->add_command(std::make_shared<cmd_vm_pop>(reg_vm::vtemp, stack_disp::bit_64));
            block->add_command(std::make_shared<cmd_vm_memread>(reg_vm::vtemp, stack_disp::bit_64));
            block->add_command(std::make_shared<cmd_vm_push>(target_temp, target_size));

            stack_displacement += target_size;
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
        const auto r_size = static_cast<reg_size>(instruction.instruction.operand_width / 8);

        push_container(container, ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMU(op_imm.value.u));
        call_instruction_handler(container, ZYDIS_MNEMONIC_PUSH, r_size, 1, true);

        stack_displacement += r_size;
        return translate_status::success;
    }

    void eagle::il::translator::base_x86_translator::finalize_translate_to_virtual()
    {
        return;
    }
}
