#pragma once
#include "virtual_machine/handlers/vm_handler_entry.h"

class vm_store_handler : public vm_handler_entry
{
public:
    vm_store_handler::vm_store_handler(vm_register_manager* manager, vm_handler_generator* handler_generator)
        : vm_handler_entry(manager, handler_generator)
    {
        supported_sizes = { reg_size::bit64 };
    };

private:
    virtual dynamic_instructions_vec construct_single(function_container container, reg_size size) override;
};