#include "virtual_machine/handlers/vm_handlers/vm_push_rflags.h"

#include "virtual_machine/vm_generator.h"

void vm_push_rflags_handler::construct_single(function_container& container, reg_size size, uint8_t operands)
{
    // TODO: there is a potential issue with this because even if we choose to ignore the rflags after whatever vm handler called this,
    // what happens to the rlags we modified? wont this hinder execution?
    // according to https://blog.back.engineering/17/05/2021/ , apparently not?
    container.add({
        zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VTEMP), ZREG(GR_RSP)),
        zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(GR_RSP), ZREG(VSP)),
        zydis_helper::enc(ZYDIS_MNEMONIC_PUSHFQ),
        zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VSP), ZREG(GR_RSP)),
        zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(GR_RSP), ZREG(VTEMP)),
    });

    create_vm_return(container);
}