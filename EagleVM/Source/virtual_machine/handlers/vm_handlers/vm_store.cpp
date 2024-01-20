#include "virtual_machine/handlers/vm_handlers/vm_store.h"

vm_store_handler::vm_store_handler()
{
    supported_sizes = { reg_size::bit64 };
}

handle_instructions vm_store_handler::construct_single(reg_size reg_size)
{
    return {};
}
