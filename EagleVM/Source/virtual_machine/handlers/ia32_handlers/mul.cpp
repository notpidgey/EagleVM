#include "virtual_machine/handlers/ia32_handlers/mul.h"

ia32_mul_handler::ia32_mul_handler()
{
    supported_sizes = { reg_size::bit64, reg_size::bit32, reg_size::bit16 };
}

instructions_vec ia32_mul_handler::construct_single(reg_size size)
{
    return instructions_vec();
}
