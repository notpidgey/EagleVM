#include "eaglevm-core/virtual_machine/handlers/vm_handlers/vm_rflags_load.h"

void vm_rflags_load_handler::construct_single(function_container& container, reg_size size, uint8_t operands, handler_override override, bool inlined)
{
    container.add({
        zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(GR_RSP), ZMEMBD(GR_RSP, -8, 8)),
        zydis_helper::enc(ZYDIS_MNEMONIC_POPFQ),
    });

    if (!inlined)
        create_vm_return(container);
}
