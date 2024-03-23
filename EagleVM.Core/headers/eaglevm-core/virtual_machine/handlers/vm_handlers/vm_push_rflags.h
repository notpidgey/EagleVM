#pragma once
#include "eaglevm-core/virtual_machine/handlers/handler/vm_handler_entry.h"

class vm_push_rflags_handler : public vm_handler_entry
{
public:
    vm_push_rflags_handler(vm_inst_regs* manager, vm_inst_handlers* handler_generator)
        : vm_handler_entry(manager, handler_generator)
    {
        handlers = {
            { bit64, 0 },
        };
    };

private:
    void construct_single(function_container& container, reg_size size, uint8_t operands, handler_override override) override;
};