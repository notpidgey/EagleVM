#pragma once
#include "virtual_machine/handlers/handler/vm_handler_entry.h"

class vm_load_handler : public vm_handler_entry
{
public:
    vm_load_handler(vm_register_manager* manager, vm_handler_generator* handler_generator)
        : vm_handler_entry(manager, handler_generator)
    {
        handlers = {
            { bit64, 0 },
            { bit32, 0 },
            { bit16, 0 },
        };
    };

private:
    virtual void construct_single(function_container& container, reg_size size, uint8_t operands) override;
};