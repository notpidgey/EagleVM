#include "virtual_machine/handlers/ia32_handlers/imul.h"

#include "virtual_machine/vm_generator.h"

void ia32_imul_handler::construct_single(function_container& container, reg_size reg_size)
{
    const vm_handler_entry* push_rflags_handler = hg_->v_handlers[MNEMONIC_VM_PUSH_RFLAGS];
    const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];
    const inst_handler_entry* push_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_PUSH];

    // pop VTEMP2
    call_vm_handler(container, pop_handler->get_handler_va(reg_size, 1));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP2), ZREG(VTEMP)));

    // pop VTEMP
    call_vm_handler(container, pop_handler->get_handler_va(reg_size, 1));

    //imul VTEMP, VTEMP2            ; imul the two registers
    auto target_temp = zydis_helper::get_bit_version(VTEMP, reg_size);
    auto target_temp2 = zydis_helper::get_bit_version(VTEMP2, reg_size);
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_IMUL, ZREG(target_temp), ZREG(target_temp2)));

    call_vm_handler(container, push_handler->get_handler_va(reg_size, 1));
    call_vm_handler(container, push_rflags_handler->get_vm_handler_va(bit64));

    create_vm_return(container);
}

void ia32_imul_handler::finalize_translate_to_virtual(const zydis_decode& decoded_instruction, function_container& container)
{
    if(decoded_instruction.operands->element_size == 1)
    {
        int current_disp = 0;

        // use the same operand twice
        encode_status status = encode_status::unsupported;
        switch(const zydis_decoded_operand& operand = decoded_instruction.operands[0]; operand.type)
        {
            case ZYDIS_OPERAND_TYPE_UNUSED:
                break;
            case ZYDIS_OPERAND_TYPE_REGISTER:
                status = encode_operand(container, decoded_instruction, operand.reg, current_disp, 1);
                break;
            case ZYDIS_OPERAND_TYPE_MEMORY:
                status = encode_operand(container, decoded_instruction, operand.mem, current_disp, 1);
                break;
            case ZYDIS_OPERAND_TYPE_POINTER:
                status = encode_operand(container, decoded_instruction, operand.ptr, current_disp);
                break;
            case ZYDIS_OPERAND_TYPE_IMMEDIATE:
                status = encode_operand(container, decoded_instruction, operand.imm, current_disp);
                break;
        }

        if(status == encode_status::unsupported)
        {
            __debugbreak();
            return;
        }
    }

    // perform the imul
    inst_handler_entry::finalize_translate_to_virtual(decoded_instruction, container);

    // top of the stack should have the rflags, since this is a real emulation of imul
    // we pop the rflags and store them on the VREG stack
    const vm_handler_entry* rlfags_handler = hg_->v_handlers[MNEMONIC_VM_POP_RFLAGS];
    call_vm_handler(container, rlfags_handler->get_vm_handler_va(bit64));

    switch(decoded_instruction.instruction.operand_count_visible)
    {
        case 1:
            // we do not support yet
            break;
        case 2:
        {
            // in two operand mode, we can only have the first operand be a OPREG type
            auto operand = decoded_instruction.operands[0];

            const vm_handler_entry* store_handler = hg_->v_handlers[MNEMONIC_VM_STORE_REG];
            const auto [base_displacement, reg_size] = rm_->get_stack_displacement(operand.reg.value);

            // the product is at the top of the stack
            // we can save to the destination register by specifying the displacement
            // and then calling store reg
            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMU(base_displacement)));
            call_vm_handler(container, store_handler->get_vm_handler_va(reg_size));
            break;
        }
        case 3:
        {
            const inst_handler_entry* mov_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_MOV];

            // since we already have the first operand virtualized as an address, we just have to call mov
            call_vm_handler(container, mov_handler->get_handler_va(static_cast<reg_size>(decoded_instruction.instruction.operand_width / 8), 2));
            break;
        }
    }
}

bool ia32_imul_handler::virtualize_as_address(const zydis_decode& inst, int index)
{
    /*
     *  https://www.felixcloutier.com/x86/imul
     *  This form requires a destination operand (the first operand) and two source operands (the second and the third operands).
     *  Here, the first source operand (which can be a general-purpose register or a memory location) is multiplied by the second source operand (an immediate value)
     *  The intermediate product (twice the size of the first source operand) is truncated and stored in the destination operand (a general-purpose register).
     */

    if(index == 0 && inst.instruction.operand_count_visible == 3)
        return true;

    return false;
}