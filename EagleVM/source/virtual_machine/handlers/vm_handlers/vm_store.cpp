#include "virtual_machine/handlers/vm_handlers/vm_store.h"

#include "virtual_machine/vm_generator.h"

void vm_store_handler::construct_single(function_container& container, reg_size reg_size)
{
    uint64_t size = reg_size;

    const vm_handler_entry* push_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_POP];
    if (reg_size == reg_size::bit64)
    {
        // lea VTEMP2, [VREGS + VTEMP]  ; load address where register is stored on the stack
        // call pop                     ; puts the value at the top of the stack into VTEMP
        // mov [VTEMP2], VTEMP          ; move value into register location
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP2), ZMEMBI(VREGS, VTEMP, 1, 8)));
        create_vm_jump(container, push_handler->get_handler_va(bit64));
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VTEMP2, 0, 8), ZREG(VTEMP)));
    }
    else if (reg_size == reg_size::bit32)
    {
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP2), ZMEMBI(VREGS, VTEMP, 1, 8)));
        create_vm_jump(container, push_handler->get_handler_va(bit64));
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VTEMP2, 0, 4), ZREG(TO32(VTEMP))));
    }
    else if (reg_size == reg_size::bit16)
    {
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP2), ZMEMBI(VREGS, VTEMP, 1, 8)));
        create_vm_jump(container, push_handler->get_handler_va(bit64));
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VTEMP2, 0, 2), ZREG(TO16(VTEMP))));
    }

    create_vm_return(container);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, container.size());
}