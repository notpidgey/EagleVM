#include "virtual_machine/handlers/ia32_handlers/sub.h"

void ia32_sub_handler::construct_single(function_container& container, reg_size reg_size)
{
    uint64_t size = reg_size;
    dynamic_instructions_vec handle_instructions;

    if (reg_size == reg_size::bit64)
    {
        //mov VTEMP, [VSP]      ; get top most value on the stack
        //sub [VSP+8], VTEMP    ; subtracts topmost value from 2nd top most value
        //pushfq                ; keep track of rflags
        //add VSP, 8            ; pop the top value off of the stack
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(VTEMP), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(VTEMP)),
            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == reg_size::bit32)
    {
        //mov VTEMP32, [VSP]
        //sub [VSP+4], VTEMP32
        //pushfq
        //add VSP, 4
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO32(VTEMP)), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(TO32(VTEMP))),
            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }
    else if (reg_size == reg_size::bit16)
    {
        //mov VTEMP16, [VSP]
        //sub [VSP+2], VTEMP16
        //pushfq
        //add VSP, 2
        handle_instructions = {
            zydis_helper::encode<ZYDIS_MNEMONIC_MOV, zydis_ereg, zydis_emem>(ZREG(TO16(VTEMP)), ZMEMBD(VSP, 0, size)),
            zydis_helper::encode<ZYDIS_MNEMONIC_SUB, zydis_emem, zydis_ereg>(ZMEMBD(VSP, size, size), ZREG(TO16(VTEMP))),
            zydis_helper::encode<ZYDIS_MNEMONIC_PUSHFQ>(),
            zydis_helper::encode<ZYDIS_MNEMONIC_ADD, zydis_ereg, zydis_eimm>(ZREG(VSP), ZIMMU(size))
        };
    }

    RETURN_EXECUTION(handle_instructions);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());

    return handle_instructions;
}
