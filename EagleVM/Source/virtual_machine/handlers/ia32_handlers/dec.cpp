#include "virtual_machine/handlers/ia32_handlers/dec.h"


dynamic_instructions_vec ia32_dec_handler::construct_single(function_container container, reg_size reg_size)
{
    uint64_t size = reg_size;
    dynamic_instructions_vec handle_instructions;

    //dec [VSP]
    handle_instructions = {
        zydis_helper::encode<ZYDIS_MNEMONIC_DEC, zydis_emem>(ZMEMBD(VSP, 0, size))
    };

    RETURN_EXECUTION(handle_instructions);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
    return handle_instructions;
}
