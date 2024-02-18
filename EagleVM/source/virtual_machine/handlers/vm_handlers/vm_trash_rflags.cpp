#include "virtual_machine/handlers/vm_handlers/vm_trash_rflags.h"

#include "virtual_machine/vm_generator.h"

void vm_trash_rflags_handler::construct_single(function_container& container, reg_size size)
{
    if(size == bit64)
    {
        // remove RFLAGS at the top of the stack
        // move RSP back so we can pop rflags
        // pop rflags
        // rsp is now at where it was before the move

        const inst_handler_entry* pop_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_POP];
        call_vm_handler(container, pop_handler->get_handler_va(bit64, 1));

        container.add({
            zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(GR_RSP), ZMEMBD(GR_RSP, -8, 8)),
            zydis_helper::enc(ZYDIS_MNEMONIC_POPFQ),
        });
    }

    create_vm_return(container);
}