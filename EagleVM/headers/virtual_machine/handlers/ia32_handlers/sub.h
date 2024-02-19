#pragma once
#include "virtual_machine/handlers/handler/inst_handler_entry.h"

class ia32_sub_handler : public inst_handler_entry
{
public:
    ia32_sub_handler(vm_register_manager* manager, vm_handler_generator* handler_generator)
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
    void finalize_translate_to_virtual(const zydis_decode& decoded_instruction, function_container& container) override;
};