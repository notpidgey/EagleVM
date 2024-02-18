#pragma once
#include "virtual_machine/handlers/handler/inst_handler_entry.h"

class ia32_movsx_handler final : public inst_handler_entry
{
public:
    ia32_movsx_handler(vm_register_manager* manager, vm_handler_generator* handler_generator)
        : inst_handler_entry(manager, handler_generator)
    {
        handlers = {
            { bit64, 2, HANDLER_BUILDER(construct_single) },
            { bit32, 2, HANDLER_BUILDER(construct_single) },
            { bit16, 2, HANDLER_BUILDER(construct_single) },
            { bit8, 2, HANDLER_BUILDER(construct_single) },
        };

        first_operand_as_ea = true;
    }

private:
    void construct_single(function_container& container, reg_size size);
    encode_status encode_operand(function_container& container, const zydis_decode& instruction, zydis_dreg op_reg, int& stack_disp, int index) override;
    encode_status encode_operand(function_container& container, const zydis_decode& instruction, zydis_dmem op_mem, int& stack_disp, int index) override;

    void upscale_temp(function_container& container, reg_size target, reg_size current);
};