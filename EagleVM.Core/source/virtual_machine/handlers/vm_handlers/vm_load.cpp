#include "eaglevm-core/virtual_machine/handlers/vm_handlers/vm_load.h"

/*
    VMLOAD
    Load the value displacement (located in VTEMP) from VREGS onto the stack
*/
namespace eagle::virt::handle
{
    void vm_load_handler::construct_single(asmbl::function_container& container, reg_size reg_size, uint8_t operands, handler_override override, bool inlined)
    {
        const inst_handler_entry* push_handler = hg_->inst_handlers[ZYDIS_MNEMONIC_PUSH];
        auto target_temp = zydis_helper::get_bit_version(VTEMP, reg_size);

        // mov VTEMP, [VREGS + VTEMP]
        // call push

        container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(target_temp), ZMEMBI(VREGS, VTEMP, 1, reg_size)));
        call_vm_handler(container, push_handler->get_handler_va(reg_size, 1));

        if (!inlined)
            create_vm_return(container);
    }
}