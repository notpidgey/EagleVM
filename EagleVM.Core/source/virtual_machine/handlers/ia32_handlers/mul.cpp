#include "eaglevm-core/virtual_machine/handlers/ia32_handlers/mul.h"

void ia32_mul_handler::construct_single(function_container& container, reg_size reg_size, uint8_t operands, handler_override override)
{
    // TODO: this is just an add handler, mul does not actually support 2 operands like this
    // we need to implement the actual mul instruction which pops AX from stack

    uint64_t size = reg_size;

    const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];
    if(reg_size == bit64)
    {
        // pop VTEMP
        call_vm_handler(container, pop_handler->get_handler_va(reg_size, 1));

        //add qword ptr [VSP], VTEMP    ; subtracts topmost value from 2nd top most value
        //pushfq                        ; keep track of rflags
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_ADD, ZMEMBD(VSP, 0, size), ZREG(VTEMP)));

        call_vm_handler(container, pop_handler->get_handler_va(reg_size, 1));
    }
    else if(reg_size == bit32)
    {
        // pop VTEMP
        call_vm_handler(container, pop_handler->get_handler_va(reg_size, 1));

        //add dword ptr [VSP], VTEMP32
        //pushfq
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_ADD, ZMEMBD(VSP, 0, size), ZREG(TO32(VTEMP))));

        call_vm_handler(container, pop_handler->get_handler_va(reg_size, 1));
    }
    else if(reg_size == bit16)
    {
        // pop VTEMP
        call_vm_handler(container, pop_handler->get_handler_va(reg_size, 1));

        //add word ptr [VSP], VTEMP16
        //pushfq
        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_ADD, ZMEMBD(VSP, 0, size), ZREG(TO16(VTEMP))));

        call_vm_handler(container, pop_handler->get_handler_va(reg_size, 1));
    }

    create_vm_return(container);
}

void ia32_mul_handler::finalize_translate_to_virtual(const zydis_decode& decoded_instruction, function_container& container)
{
    const vm_handler_entry* push_rflags_handler = hg_->v_handlers[MNEMONIC_VM_RFLAGS_LOAD];
    call_vm_handler(container, push_rflags_handler->get_vm_handler_va(bit64));

    inst_handler_entry::finalize_translate_to_virtual(decoded_instruction, container);

    const vm_handler_entry* rflag_handler = hg_->v_handlers[MNEMONIC_VM_RFLAGS_ACCEPT];
    call_vm_handler(container, rflag_handler->get_vm_handler_va(bit64));
}