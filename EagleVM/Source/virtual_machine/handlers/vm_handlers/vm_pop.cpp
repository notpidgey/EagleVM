#include "virtual_machine/handlers/vm_handlers/vm_pop.h"

vm_pop_handler::vm_pop_handler()
{
    supported_sizes = { reg_size::bit64, reg_size::bit32, reg_size::bit16 };
}

instructions_vec vm_pop_handler::construct_single(reg_size reg_size)
{
    uint64_t size = reg_size;
    instructions_vec handle_instructions;

    //mov VTEMP, [vsp]
    //add VSP, reg_size
    if (reg_size == reg_size::bit64)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == reg_size::bit32)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO32(VTEMP)), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == reg_size::bit16)
    {
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO16(VTEMP)), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }

    RETURN_EXECUTION(handle_instructions);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
    return handle_instructions;
}
