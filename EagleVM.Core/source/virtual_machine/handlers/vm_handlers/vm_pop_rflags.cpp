#include "eaglevm-core/virtual_machine/handlers/vm_handlers/vm_pop_rflags.h"

void vm_pop_rflags_handler::construct_single(function_container& container, reg_size size, uint8_t operands)
{
    container.add({
        zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZREG(GR_RSP)),
        zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(GR_RSP), ZREG(VSP)),
        zydis_helper::enc(ZYDIS_MNEMONIC_POPFQ),
        zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VSP), ZREG(GR_RSP)),
        zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(GR_RSP), ZREG(VTEMP)),
        zydis_helper::enc(ZYDIS_MNEMONIC_PUSHFQ),
        zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(GR_RSP), ZMEMBD(GR_RSP, 8, 8))
    });

    create_vm_return(container);
}