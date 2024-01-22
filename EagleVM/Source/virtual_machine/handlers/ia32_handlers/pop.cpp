#include "virtual_machine/handlers/ia32_handlers/pop.h"

void ia32_pop_handler::construct_single(function_container& container, reg_size reg_size)
{
    uint64_t size = reg_size;
    dynamic_instructions_vec handle_instructions;

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

    create_vm_return(container);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
}
