#include "virtual_machine/handlers/ia32_handlers/inc.h"

#include "virtual_machine/vm_generator.h"

void ia32_inc_handler::construct_single(function_container& container, reg_size reg_size)
{
    uint64_t size = reg_size;
    dynamic_instructions_vec handle_instructions;

    //inc [VSP]
    handle_instructions = {
        zydis_helper::encode<ZYDIS_MNEMONIC_INC, zydis_emem>(ZMEMBD(VSP, 0, size))
    };

    create_vm_return(container);
    std::printf("%3c %-17s %-10zi\n", zydis_helper::reg_size_to_string(reg_size), __func__, handle_instructions.size());
}