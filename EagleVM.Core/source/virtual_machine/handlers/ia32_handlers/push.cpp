#include "eaglevm-core/virtual_machine/handlers/ia32_handlers/push.h"

void ia32_push_handler::construct_single(function_container& container, reg_size reg_size, uint8_t operands, handler_override override)
{
    int64_t size = reg_size;
    dynamic_instructions_vec handle_instructions;

    // sub VSP, reg_size
    // mov [vsp], VTEMP

    const zydis_register target_temp = zydis_helper::get_bit_version(VTEMP, reg_size);
    container.add({
        zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VSP), ZMEMBD(VSP, -size, 8)),
        zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZMEMBD(VSP, 0, size), ZREG(target_temp))
    });

    create_vm_return(container);
}