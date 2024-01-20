#include "virtual_machine/handlers/ia32_handlers/div.h"

ia32_div_handler::ia32_div_handler()
{
    supported_sizes = { reg_size::bit64, reg_size::bit32, reg_size::bit16 };
}

handle_instructions ia32_div_handler::construct_single(reg_size size)
{
    return handle_instructions();
}
