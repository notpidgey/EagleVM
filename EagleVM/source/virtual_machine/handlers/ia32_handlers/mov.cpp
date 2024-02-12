#include "virtual_machine/handlers/ia32_handlers/mov.h"

#include "virtual_machine/vm_generator.h"

void ia32_mov_handler::construct_single(function_container& container, reg_size size)
{
    // value we want to move should be located at the top of the stack
    // the address we want to move TO should be located right below it
    if(size == bit64)
    {
        const vm_handler_entry* push_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_POP];

        // pop  ; load the value into VTEMP
        // mov [VSP], VTEMP
        call_vm_handler(container, push_handler->get_handler_va(size));
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, 8), ZREG(VTEMP)));
    }
    else if(size == bit32)
    {
        const vm_handler_entry* push_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_POP];

        // pop  ; load the value into VTEMP
        // mov [VSP], VTEMP
        call_vm_handler(container, push_handler->get_handler_va(size));
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, 4), ZREG(TO32(VTEMP))));
    }
    else if(size == bit16)
    {
        const vm_handler_entry* push_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_POP];

        // pop  ; load the value into VTEMP
        // mov [VSP], VTEMP
        call_vm_handler(container, push_handler->get_handler_va(size));
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, 2), ZREG(TO16(VTEMP))));
    }
}