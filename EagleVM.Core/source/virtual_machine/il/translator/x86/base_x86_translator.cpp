#include "eaglevm-core/virtual_machine/il/translator/x86/base_x86_translator.h"
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"
#include "eaglevm-core/virtual_machine/il/commands/cmd_push.h"
#include "eaglevm-core/virtual_machine/il/commands/cmd_context_load.h"
#include "eaglevm-core/virtual_machine/il/commands/cmd_handler_call.h"
#include "eaglevm-core/virtual_machine/il/commands/cmd_pop.h"

namespace eagle::il::translator
{
    base_x86_translator::base_x86_translator(il_bb_ptr block_ptr, asmbl::x86::dec::inst_info decode)
        : block(std::move(block_ptr)), inst(decode.instruction)
    {
        stack_displacement = 0;
        std::ranges::copy(decode.operands, std::begin(operands));
    }

    base_x86_translator::base_x86_translator(asmbl::x86::dec::inst_info decode)
        : block(std::make_shared<il_bb>(false)), inst(decode.instruction)
    {
        stack_displacement = 0;
        std::ranges::copy(decode.operands, std::begin(operands));
    }

    bool base_x86_translator::translate_to_il(uint64_t original_rva)
    {
        for (uint8_t i = 0; i < inst.; i++)
        {
            translate_status status = translate_status::unsupported;
            switch (const asmbl::x86::dec::operand& operand = operands[i]; operand.type)
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

    int base_x86_translator::get_op_action(asmbl::x86::operand_t op_type, int index)
    {
        return action_value;
    }

    translate_status base_x86_translator::encode_operand(asmbl::x86::dec::op_reg op_reg, uint8_t idx)
    {
        // TODO: what about cases where we have RSP as the register?

        // we will always want the address/offset right at the bottom of the stack
        // in the future this might change, but for now it will stay like this

        const int action = get_op_action(ZYDIS_OPERAND_TYPE_REGISTER, idx);
        if (action & action_address)
            load_reg_address(op_reg);

        if (action & action_reg_offset)
            load_reg_offset(op_reg);

        if (action & action_value)
            load_reg_value(op_reg);

        return translate_status::success;
    }

    translate_status base_x86_translator::encode_operand(asmbl::x86::dec::op_mem op_mem, uint8_t idx)
    {
        if (op_mem.type != ZYDIS_MEMOP_TYPE_MEM && op_mem.type != ZYDIS_MEMOP_TYPE_AGEN)
            return translate_status::unsupported;

        //[base + index * scale + disp]

        //1. begin with loading the base register
        //mov VTEMP, imm
        //jmp VM_LOAD_REG
        asmbl::code_label* rip_label;
        {
            if (op_mem.base == ZYDIS_REGISTER_RSP)
            {
                cmd_vm_push_ptr push = std::make_shared<cmd_vm_push>(reg_vm::vsp, reg_size::b64);
                if (stack_displacement)
                {
                    // todo: add stack displacement to push
                }

                block->add_command(push);
            }
            else if (op_mem.base == ZYDIS_REGISTER_RIP)
            {
                block->add_command(std::make_shared<cmd_context_load>(asmbl::x86::rip, asmbl::x86::reg_size::gpr_64));
            }
            else
            {
                block->add_command(std::make_shared<cmd_context_load>(op_mem.base, asmbl::x86::reg_size::gpr_64));
            }
        }

        //2. load the index register and multiply by scale
        //mov VTEMP, imm    ;
        //jmp VM_LOAD_REG   ; load value of INDEX reg to the top of the VSTACK
        if (op_mem.index != ZYDIS_REGISTER_NONE)
        {
            block->add_command(std::make_shared<cmd_context_load>(op_mem.index, reg_size::b64));
        }

        if (op_mem.scale != 0)
        {
            //mov VTEMP, imm    ;
            //jmp VM_PUSH       ; load value of SCALE to the top of the VSTACK
            //jmp VM_MUL        ; multiply INDEX * SCALE
            block->add_command(std::make_shared<cmd_vm_push>(op_mem.scale, reg_size::b64));
            block->add_command(std::make_shared<cmd_handler_call>(call_type::inst_handler,
                asmbl::x86::IMUL, 2, asmbl::x86::reg_size::gpr_64));
        }

        if (op_mem.index != ZYDIS_REGISTER_NONE)
        {
            block->add_command(std::make_shared<cmd_handler_call>(call_type::inst_handler,
                asmbl::x86::ADD, 2, asmbl::x86::reg_size::gpr_64));
        }

        if (op_mem.disp.has_displacement)
        {
            // 3. load the displacement and add
            // we can do this with some trickery using LEA so we dont modify rflags

            if (op_mem.base == ZYDIS_REGISTER_RIP)
            {
                // since this is RIP relative we first want to calculate where the original instruction is trying to access
                auto [target, _] = asmbl::x86::calc_relative_rva(instruction, orig_rva, index);

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
                // pop current value into VTEMP
                // lea VTEMP, [VTEMP +- imm]
                // push

                call_instruction_handler(container, ZYDIS_MNEMONIC_POP, bit64, 1, true);
                push_container(container, ZYDIS_MNEMONIC_LEA, ZREG(VTEMP), ZMEMBD(VTEMP, op_mem.disp.value, 8));
                call_instruction_handler(container, ZYDIS_MNEMONIC_PUSH, bit64, 1, true);
            }
        }

        // for memory operands we will only ever need one kind of action
        // there has to be a better and cleaner way of doing this, but i have not thought of it yet
        // for now it will kind of just be an assumption

        const int action = get_op_action(ZYDIS_OPERAND_TYPE_MEMORY, idx);
        if (action & action_address || op_mem.type == ZYDIS_MEMOP_TYPE_AGEN)
        {
            stack_displacement += bit64;
        }
        else if (action & action_value)
        {
            // by default, this will be dereferenced and we will get the value at the address,
            const reg_size target_size = static_cast<reg_size>(inst.operand_width / 8);

            // this means we are working with the second operand
            const reg target_temp = get_bit_version(VTEMP, target_size);

            block->add_command(std::make_shared<cmd_vm_pop>(reg_vm::vtemp, reg_size::b64));
            push_container(container, ZYDIS_MNEMONIC_MOV, ZREG(target_temp), ZMEMBD(VTEMP, 0, target_size));
            block->add_command(std::make_shared<cmd_vm_push>(reg_vm::vtemp, reg_size::b64));

            stack_displacement += target_size;
        }
        else
        {
            // too lazy to implement multiple options for now, but this shouldnt happen
            return translate_status::unsupported;
        }

        return translate_status::success;
    }

    translate_status base_x86_translator::encode_operand(asmbl::x86::dec::op_ptr op_ptr, uint8_t idx)
    {
        // not a supported operand
        return translate_status::unsupported;
    }

    translate_status base_x86_translator::encode_operand(asmbl::x86::dec::op_imm op_imm, uint8_t idx)
    {
        auto [stack_disp, orig_rva, index] = context;
        const auto r_size = static_cast<reg_size>(instruction.instruction.operand_width / 8);

        push_container(container, ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMU(op_imm.value.u));
        call_instruction_handler(container, ZYDIS_MNEMONIC_PUSH, r_size, 1, true);

        *stack_disp += r_size;
        return translate_status::success;
    }

    void eagle::il::translator::base_x86_translator::finalize_translate_to_virtual()
    {
    }

    void base_x86_translator::load_reg_address(asmbl::x86::dec::op_reg reg)
    {
        block->add_command(std::make_shared<cmd_context_load>());
        stack_displacement += bit64;
    }

    void base_x86_translator::load_reg_offset(asmbl::x86::dec::op_reg reg)
    {
        const auto [displacement, size] = rm_->get_stack_displacement(op_reg.value);

        // this means we want to put the address of of the target register at the top of the stack
        // mov VTEMP, DISPLACEMENT
        // push

        push_container(container, ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMS(displacement));
        call_instruction_handler(container, ZYDIS_MNEMONIC_PUSH, bit32, 1, true); // always 32 bit bececause its an imm

        stack_displacement += bit64;
    }

    void base_x86_translator::load_reg_value(asmbl::x86::dec::op_reg reg)
    {
        const auto [displacement, size] = rm_->get_stack_displacement(op_reg.value);

        // this routine will load the register value to the top of the VSTACK
        // mov VTEMP, -8
        // call VM_LOAD_REG

        push_container(container, ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMS(displacement));
        call_virtual_handler(container, MNEMONIC_VM_LOAD_REG, zydis_helper::get_reg_size(op_reg.value), true);

        stack_disp += zydis_helper::get_reg_size(op_reg.value);
    }
}
