#include "virtual_machine/handlers/vm_handlers/vm_load.h"

vm_load_handler::vm_load_handler()
{
    supported_sizes = { reg_size::bit64 };
}

handle_instructions vm_load_handler::construct_single(reg_size reg_size)
{
    uint64_t size = reg_size;
    handle_instructions handle_instructions;

    if (reg_size == reg_size::bit64)
    {
        //mov VTEMP, [VREGS+VTEMP]      ; get address of register
        //mov VTEMP, qword ptr [VTEMP]  ; get register value
        //sub VSP, size                 ; increase VSP
        //mov [VSP], VTEMP              ; push value to stack
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBI(VREGS, VTEMP, 1, 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VREGS, 0, reg_size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(VTEMP))
        };
    }
    else if (reg_size == reg_size::bit32)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBI(VREGS, VTEMP, 1, 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO32(VTEMP)), ZMEMBD(VREGS, 0, reg_size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(TO32(VTEMP)))
        };
    }
    else if (reg_size == reg_size::bit16)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBI(VREGS, VTEMP, 1, 8)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO16(VTEMP)), ZMEMBD(VREGS, 0, reg_size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(TO16(VTEMP)))
        };
    }

    RETURN_EXECUTION(handle_instructions);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
    return handle_instructions;
}
