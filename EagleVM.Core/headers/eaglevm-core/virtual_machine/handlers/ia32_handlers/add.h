#pragma once
#include "eaglevm-core/virtual_machine/handlers/handler/inst_handler_entry.h"

class ia32_add_handler final : public inst_handler_entry
{
public:
    ia32_add_handler(vm_inst_regs* manager, vm_inst_handlers* handler_generator)
        : inst_handler_entry(manager, handler_generator)
    {
        handlers = {
            { bit64, 2 },
            { bit32, 2 },
            { bit16, 2 },
        };
    };

private:
    void construct_single(function_container& container, reg_size size, uint8_t operands) override;
    void finalize_translate_to_virtual(const zydis_decode& decoded_instruction, function_container& container) override;
    vm_op_action get_virtualize_action(const zydis_decode& inst, int index) override;
};