#include "virtual_machine/handlers/ia32_handlers/add.h"

dynamic_instructions_vec ia32_add_handler::construct_single(function_container container, reg_size reg_size)
{
    uint64_t size = reg_size;
    dynamic_instructions_vec handle_instructions;

    //sub VSP, reg_size
    //mov [vsp], VTEMP
    if(reg_size == reg_size::bit64)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(VTEMP))
        };
    }
    else if(reg_size == reg_size::bit32)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(TO32(VTEMP)))
        };
    }
    else if(reg_size == reg_size::bit16)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_emem, zydis_ereg>(ZMEMBD(VSP, 0, size), ZREG(TO16(VTEMP)))
        };
    }

    RETURN_EXECUTION(handle_instructions);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
    return handle_instructions;
}