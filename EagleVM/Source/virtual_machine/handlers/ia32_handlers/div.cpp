#include "virtual_machine/handlers/ia32_handlers/div.h"

void ia32_div_handler::construct_single(function_container& container, reg_size size)
{
    return dynamic_instructions_vec();
}
