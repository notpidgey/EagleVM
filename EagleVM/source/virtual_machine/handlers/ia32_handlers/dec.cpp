#include "virtual_machine/handlers/ia32_handlers/dec.h"

#include "virtual_machine/vm_generator.h"

void ia32_dec_handler::construct_single(function_container& container, reg_size reg_size)
{
    uint64_t size = reg_size;

    const vm_handler_entry* pop_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_POP];
    if(reg_size == reg_size::bit64)
    {
        // pop VTEMP
        create_vm_jump(container, pop_handler->get_handler_va(reg_size));

        //dec VTEMP                     ; dec popped value
        //pushfq                        ; keep track of rflags
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_DEC, zydis_ereg>(ZREG(VTEMP)),
            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
        });
    }
    else if(reg_size == reg_size::bit32)
    {
        // pop VTEMP
        create_vm_jump(container, pop_handler->get_handler_va(reg_size));

        //dec VTEMP
        //pushfq
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_DEC, zydis_ereg>(ZREG(TO32(VTEMP))),
            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
        });
    }
    else if(reg_size == reg_size::bit16)
    {
        // pop VTEMP
        create_vm_jump(container, pop_handler->get_handler_va(reg_size));

        //add word ptr [VSP], VTEMP16
        //pushfq
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_DEC, zydis_ereg>(ZREG(TO16(VTEMP))),
            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
        });
    }

    create_vm_return(container);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, container.size());
}
