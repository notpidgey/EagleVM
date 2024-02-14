#include "virtual_machine/handlers/vm_handlers/vm_load.h"

#include "virtual_machine/vm_generator.h"

/*
    VMLOAD
    Load the value displacement (located in VTEMP) from VREGS onto the stack
*/
void vm_load_handler::construct_single(function_container& container, reg_size reg_size)
{
    uint64_t size = reg_size;
    dynamic_instructions_vec handle_instructions;

    const vm_handler_entry* push_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_PUSH];
    if (reg_size == reg_size::bit64)
    {
        // mov VTEMP, [VREGS + VTEMP]
        // call push
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBI(VREGS, VTEMP, 1, 8)));
        call_vm_handler(container, push_handler->get_handler_va(bit64));
    }
    else if (reg_size == reg_size::bit32)
    {
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO32(VTEMP)), ZMEMBI(VREGS, VTEMP, 1, 4)));
        call_vm_handler(container, push_handler->get_handler_va(bit64));
    }
    else if (reg_size == reg_size::bit16)
    {
        container.add(zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO16(VTEMP)), ZMEMBI(VREGS, VTEMP, 1, 2)));
        call_vm_handler(container, push_handler->get_handler_va(bit64));
    }

    create_vm_return(container);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
}
