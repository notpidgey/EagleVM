#include "virtual_machine/handlers/vm_handlers/vm_load.h"

#include "virtual_machine/vm_generator.h"

void vm_load_handler::construct_single(function_container& container, reg_size reg_size)
{
    uint64_t size = reg_size;
    dynamic_instructions_vec handle_instructions;

    if (reg_size == reg_size::bit64)
    {
        //mov VTEMP, [VREGS+VTEMP]      ; get address of register
        //mov VTEMP, qword ptr [VTEMP]  ; get register value
        //sub VSP, size                 ; increase VSP
        //mov [VSP], VTEMP              ; push value to stack
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBI(VREGS, VTEMP, 1, 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VREGS, 0, reg_size)),
        });

        const vm_handler_entry* push_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_PUSH];
        create_vm_jump(container, push_handler->get_handler_va(bit64));
    }
    else if (reg_size == reg_size::bit32)
    {
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBI(VREGS, VTEMP, 1, 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO32(VTEMP)), ZMEMBD(VREGS, 0, reg_size)),
        });

        const vm_handler_entry* push_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_PUSH];
        create_vm_jump(container, push_handler->get_handler_va(bit32));
    }
    else if (reg_size == reg_size::bit16)
    {
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBI(VREGS, VTEMP, 1, 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO16(VTEMP)), ZMEMBD(VREGS, 0, reg_size)),
        });

        const vm_handler_entry* push_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_PUSH];
        create_vm_jump(container, push_handler->get_handler_va(bit16));
    }

    create_vm_return(container);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
}
