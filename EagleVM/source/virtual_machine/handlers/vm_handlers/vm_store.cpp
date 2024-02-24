#include "virtual_machine/handlers/vm_handlers/vm_store.h"

#include "virtual_machine/vm_generator.h"

void vm_store_handler::construct_single(function_container& container, reg_size reg_size, uint8_t operands)
{
    const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];

    // lea VTEMP2, [VREGS + VTEMP]  ; load address where register is stored on the stack
    // call pop                     ; puts the value at the top of the stack into VTEMP
    // mov [VTEMP2], VTEMP          ; move value into register location

    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VTEMP2), ZMEMBI(VREGS, VTEMP, 1, 8)));
    call_vm_handler(container, pop_handler->get_handler_va(reg_size, 1));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZMEMBD(VTEMP2, 0, 8), ZREG(VTEMP)));

    create_vm_return(container);
}