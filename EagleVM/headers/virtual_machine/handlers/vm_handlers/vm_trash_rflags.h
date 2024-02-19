#pragma once
#include "virtual_machine/handlers/handler/vm_handler_entry.h"

class vm_trash_rflags_handler : public vm_handler_entry
{
public:
    vm_trash_rflags_handler(vm_register_manager* manager, vm_handler_generator* handler_generator)
        : vm_handler_entry(manager, handler_generator)
    {
        handlers = {
            { bit64, 0 },
        };
    }

private:
    void construct_single(function_container& container, reg_size size, uint8_t operands) override;
};