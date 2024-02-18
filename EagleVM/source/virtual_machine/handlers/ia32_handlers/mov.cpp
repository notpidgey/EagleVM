#include "virtual_machine/handlers/ia32_handlers/mov.h"

#include "virtual_machine/vm_generator.h"

void ia32_mov_handler::construct_single(function_container& container, reg_size size)
{
    // value we want to move should be located at the top of the stack
    // the address we want to move TO should be located right below it

    const vm_handler_entry* pop_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_POP];

    // pop                  ; load the value into VTEMP
    // mov VTEMP2, [VSP]    ; now the address at VSP is the address we want to write to
    // mov [VTEMP2], VTEMP  ; write VTEMP into the address we have
    // pop                  ; we want to maintain the stack
    call_vm_handler(container, pop_handler->get_handler_va(size));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP2), ZMEMBD(VSP, 0, 8)));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZMEMBD(VTEMP2, 0, 8), ZREG(VTEMP)));
    call_vm_handler(container, pop_handler->get_handler_va(bit64));

    create_vm_return(container);
}