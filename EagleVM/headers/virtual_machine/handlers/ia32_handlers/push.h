#pragma once
#include "virtual_machine/handlers/vm_handler_entry.h"

class ia32_push_handler : public vm_handler_entry
{
public:
    ia32_push_handler(vm_register_manager* manager, vm_handler_generator* handler_generator)
        : vm_handler_entry(manager, handler_generator)
    {
        supported_sizes = { bit64, bit32, bit16, bit8 };
    };

private:
    void construct_single(function_container& container, reg_size size) override;
};
