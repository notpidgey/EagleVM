#pragma once
#include "eaglevm-core/virtual_machine/handlers/handler/inst_handler_entry.h"

class ia32_inc_handler : public inst_handler_entry
{
public:
    ia32_inc_handler(vm_inst_regs* manager, vm_inst_handlers* handler_generator)
        : inst_handler_entry(manager, handler_generator)
    {
        handlers = {
            { bit64, 1 },
            { bit32, 1 },
            { bit16, 1 },
        };
    };

private:
    void construct_single(function_container& container, reg_size size, uint8_t operands) override;
    void finalize_translate_to_virtual(const zydis_decode& decoded_instruction, function_container& container) override;

    int get_op_action(const zydis_decode& inst, zyids_operand_t op_type, int index) override;
};