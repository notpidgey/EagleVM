#pragma once
#include "virtual_machine/handlers/handler/inst_handler_entry.h"

class ia32_div_handler : public inst_handler_entry
{
public:
    ia32_div_handler(vm_register_manager* manager, vm_handler_generator* handler_generator)
        : inst_handler_entry(manager, handler_generator)
    {
        handlers = {
            { bit64, 2, HANDLER_BUILDER(construct_single) },
            { bit32, 2, HANDLER_BUILDER(construct_single) },
            { bit16, 2, HANDLER_BUILDER(construct_single) },
        };
    };

private:
    void construct_single(function_container& container, reg_size size);
};