#include "virtual_machine/handlers/ia32_handlers/movsx.h"

#include "virtual_machine/vm_generator.h"

void ia32_movsx_handler::construct_single(function_container& container, reg_size size)
{
    const vm_handler_entry* pop_handler = hg_->vm_handlers[ZYDIS_MNEMONIC_POP];
    if(size == bit64)
    {
        // VTEMP2 should contain either a 1 or 0
        // Depending on that value, we will fill lower "size" (64/32/16) bits in the VTEMP1 register with that value
        // Next, depending on the register size, we will copy

        call_vm_handler(container, pop_handler->get_handler_va(size));
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_MOVSX, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VTEMP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(GR_RSP), ZMEMBD(GR_RSP, 8, 8))
        });
        call_vm_handler(container, pop_handler->get_handler_va(size));
    }
    else if(size == bit32)
    {
        call_vm_handler(container, pop_handler->get_handler_va(size));
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_MOVSX, zydis_ereg, zydis_emem>(ZREG(TO32(VTEMP)), ZMEMBD(VTEMP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(GR_RSP), ZMEMBD(GR_RSP, 8, 8))
        });
        call_vm_handler(container, pop_handler->get_handler_va(size));
    }
    else if(size == bit16)
    {
        call_vm_handler(container, pop_handler->get_handler_va(size));
        container.add({
            zydis_helper::encode<ZYDIS_MNEMONIC_MOVSX, zydis_ereg, zydis_emem>(ZREG(TO16(VTEMP)), ZMEMBD(VTEMP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(GR_RSP), ZMEMBD(GR_RSP, 8, 8))
        });
        call_vm_handler(container, pop_handler->get_handler_va(size));
    }

    create_vm_return(container);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(size), __func__, container.size());
}