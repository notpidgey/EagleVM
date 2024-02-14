#include "virtual_machine/handlers/ia32_handlers/imul.h"

#include "virtual_machine/vm_generator.h"

void ia32_imul_handler::construct_single(function_container& container, reg_size reg_size)
{
    uint64_t size = reg_size;

    const vm_handler_entry* pop_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_POP];
    const vm_handler_entry* push_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_PUSH];
    if(reg_size == bit64)
    {
        // pop VTEMP2
        call_vm_handler(container, pop_handler->get_handler_va(reg_size));
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VTEMP2), ZREG(VTEMP)));

        // pop VTEMP
        call_vm_handler(container, pop_handler->get_handler_va(reg_size));

        //imul VTEMP, VTEMP2            ; imul the two registers
        //pushfq                        ; keep track of rflags
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_IMUL, zydis_ereg, zydis_ereg>(ZREG(VTEMP), ZREG(VTEMP2)),

            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(GR_RSP), ZMEMBD(GR_RSP, 8, 8))
        });

        call_vm_handler(container, push_handler->get_handler_va(reg_size));
    }
    else if(reg_size == bit32)
    {
        // pop VTEMP2
        call_vm_handler(container, pop_handler->get_handler_va(reg_size));
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VTEMP2), ZREG(VTEMP)));

        // pop VTEMP
        call_vm_handler(container, pop_handler->get_handler_va(reg_size));

        //imul VTEMP, VTEMP2            ; imul the two registers
        //pushfq                        ; keep track of rflags
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_IMUL, zydis_ereg, zydis_ereg>(ZREG(TO32(VTEMP)), ZREG(TO32(VTEMP2))),

            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(GR_RSP), ZMEMBD(GR_RSP, 8, 8))
        });

        call_vm_handler(container, push_handler->get_handler_va(reg_size));
    }
    else if(reg_size == bit16)
    {
        // pop VTEMP2
        call_vm_handler(container, pop_handler->get_handler_va(reg_size));
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_ereg>(ZREG(VTEMP2), ZREG(VTEMP)));

        // pop VTEMP
        call_vm_handler(container, pop_handler->get_handler_va(reg_size));

        //imul VTEMP, VTEMP2            ; imul the two registers
        //pushfq                        ; keep track of rflags
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_IMUL, zydis_ereg, zydis_ereg>(ZREG(TO16(VTEMP)), ZREG(TO16(VTEMP2))),

            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(GR_RSP), ZMEMBD(GR_RSP, 8, 8))
        });

        call_vm_handler(container, push_handler->get_handler_va(reg_size));
    }

    create_vm_return(container);
}

void ia32_imul_handler::finalize_translate_to_virtual(const zydis_decode& decoded_instruction, function_container& container)
{
    if(decoded_instruction.operands->element_size == 1)
    {
        // use the same operand twice
        encode_status status = encode_status::unsupported;
        switch(const zydis_decoded_operand& operand = decoded_instruction.operands[0]; operand.type)
        {
            case ZYDIS_OPERAND_TYPE_UNUSED:
                break;
            case ZYDIS_OPERAND_TYPE_REGISTER:
                status = encode_operand(container, decoded_instruction, operand.reg, 1);
            break;
            case ZYDIS_OPERAND_TYPE_MEMORY:
                status = encode_operand(container, decoded_instruction, operand.mem, 1);
            break;
            case ZYDIS_OPERAND_TYPE_POINTER:
                status = encode_operand(container, decoded_instruction, operand.ptr);
            break;
            case ZYDIS_OPERAND_TYPE_IMMEDIATE:
                status = encode_operand(container, decoded_instruction, operand.imm);
            break;
        }

        if(status == encode_status::unsupported)
        {
            __debugbreak();
            return;
        }
    }

    // perform the imul
    vm_handler_entry::finalize_translate_to_virtual(decoded_instruction, container);

    auto operand = decoded_instruction.operands[0];
    switch(operand.type)
    {
        case ZYDIS_OPERAND_TYPE_REGISTER:
        {
            const vm_handler_entry* store_handler = hg_->vm_handlers[MNEMONIC_VM_STORE_REG];
            const auto [base_displacement, reg_size] = rm_->get_stack_displacement(operand.reg.value);

            // the product is at the top of the stack
            // we can save to the destination register by specifying the displacement
            // and then calling store reg
            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMU(base_displacement)));
            call_vm_handler(container, store_handler->get_handler_va(reg_size));
        }
        break;
        case ZYDIS_OPERAND_TYPE_MEMORY:
        {
            // the product is at the top of the stack
            // we can save to the destination register by specifying the displacement

            const reg_size reg_size = zydis_helper::get_reg_size(operand.mem.base);
            container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(
                ZMEMBD(VTEMP, 0, reg_size),
                ZREG(zydis_helper::get_bit_version(VTEMP, reg_size))
            ));
        }
        break;
        default: __debugbreak();
    }
}