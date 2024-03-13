#pragma once
#include "virtual_machine/handlers/handler/inst_handler_entry.h"

class ia32_lea_handler : public inst_handler_entry
{
public:
    ia32_lea_handler(vm_inst_regs* manager, vm_inst_handlers* handler_generator)
        : inst_handler_entry(manager, handler_generator)
    {
        handlers = {
            { bit64, 2 },
            { bit32, 2 },
            { bit16, 2 },
            { bit8, 2 },
        };
    };

private:
    void construct_single(function_container& container, reg_size size, uint8_t operands) override;
    bool virtualize_as_address(const zydis_decode& inst, int index) override;
};