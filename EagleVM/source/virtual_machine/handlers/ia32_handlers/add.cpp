#include "virtual_machine/handlers/ia32_handlers/add.h"

#include "virtual_machine/vm_generator.h"

void ia32_add_handler::construct_single(function_container& container, reg_size reg_size)
{
    uint64_t size = reg_size;

    const vm_handler_entry* push_rflags_handler = hg_->vm_handlers[MNEMONIC_VM_PUSH_RFLAGS];
    const vm_handler_entry* pop_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_POP];

    const zydis_register target_temp = zydis_helper::get_bit_version(VTEMP, reg_size);

    // pop VTEMP
    call_vm_handler(container, pop_handler->get_handler_va(reg_size));

    // sub qword ptr [VSP], VTEMP       ; subtracts topmost value from 2nd top most value
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_ADD, ZMEMBD(VSP, 0, size), ZREG(target_temp)));

    // push rflags                      ; push rflags in case we want to accept these changes
    call_vm_handler(container, push_rflags_handler->get_handler_va(bit64));

    create_vm_return(container);
}

void ia32_add_handler::finalize_translate_to_virtual(const zydis_decode& decoded_instruction, function_container& container)
{
    vm_handler_entry::finalize_translate_to_virtual(decoded_instruction, container);

    auto operand = decoded_instruction.operands[0];
    switch(operand.type)
    {
        case ZYDIS_OPERAND_TYPE_REGISTER:
        {
            const vm_handler_entry* store_handler = hg_->vm_handlers[MNEMONIC_VM_STORE_REG];
            const auto [base_displacement, reg_size] = rm_->get_stack_displacement(operand.reg.value);

            // the sum is at the top of the stack
            // we can save to the destination register by specifying the displacement
            // and then calling store reg
            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZIMMU(base_displacement)));
            call_vm_handler(container, store_handler->get_handler_va(reg_size));
        }
        break;
        case ZYDIS_OPERAND_TYPE_MEMORY:
        {
            // the sum is at the top of the stack
            // we can save to the destination register by specifying the displacement

            const reg_size reg_size = zydis_helper::get_reg_size(operand.mem.base);
            container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV,
                ZMEMBD(VTEMP, 0, reg_size),
                ZREG(zydis_helper::get_bit_version(VTEMP, reg_size))
            ));
        }
        break;
        default: __debugbreak();
    }

    // accept changes to rflags
    const vm_handler_entry* store_handler = hg_->vm_handlers[MNEMONIC_VM_POP_RFLAGS];
    call_vm_handler(container, store_handler->get_handler_va(bit64));
}