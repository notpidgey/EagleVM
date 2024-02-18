#pragma once
#include "virtual_machine/handlers/handler/base_handler_entry.h"

class vm_handler_entry : public base_handler_entry
{
public:
    vm_handler_entry(vm_register_manager* manager, vm_handler_generator* handler_generator)
        : base_handler_entry(manager, handler_generator) { }

    code_label* get_vm_handler_va(reg_size width) const;
};