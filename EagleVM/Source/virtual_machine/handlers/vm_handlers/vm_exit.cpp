#include "virtual_machine/handlers/vm_handlers/vm_exit.h"

vm_exit_handler::vm_exit_handler()
{
}

handle_instructions vm_exit_handler::construct_single(reg_size size)
{
    return handle_instructions();
}
