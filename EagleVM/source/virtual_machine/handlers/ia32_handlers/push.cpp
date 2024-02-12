#include "virtual_machine/handlers/ia32_handlers/push.h"

void ia32_push_handler::construct_single(function_container& container, reg_size reg_size)
{
    int64_t size = reg_size;
    dynamic_instructions_vec handle_instructions;

    //sub VSP, reg_size
    //mov [vsp], VTEMP
    if (reg_size == bit64)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VSP), ZMEMBD(VSP, -size, 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(VTEMP))
        };
    }
    else if (reg_size == bit32)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VSP), ZMEMBD(VSP, -size, 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(TO32(VTEMP)))
        };
    }
    else if (reg_size == bit16)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_LEA, zydis_ereg, zydis_emem>(ZREG(VSP), ZMEMBD(VSP, -size, 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(TO16(VTEMP)))
        };
    }

    create_vm_return(container);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
}
