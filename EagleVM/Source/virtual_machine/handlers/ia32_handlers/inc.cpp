#include "virtual_machine/handlers/ia32_handlers/inc.h"

ia32_inc_handler::ia32_inc_handler()
{
    supported_sizes = { reg_size::bit64, reg_size::bit32, reg_size::bit16, reg_size::bit8 };
}

instructions_vec ia32_inc_handler::construct_single(reg_size reg_size)
{
    uint64_t size = reg_size;
    instructions_vec handle_instructions;

    //inc [VSP]
    handle_instructions = {
        zydis_helper::encode<ZYDIS_MNEMONIC_INC, zydis_emem>(ZMEMBD(VSP, 0, size))
    };

    RETURN_EXECUTION(handle_instructions);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
    return handle_instructions;
}
