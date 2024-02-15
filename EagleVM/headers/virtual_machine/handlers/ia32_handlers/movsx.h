#pragma once
#include "virtual_machine/handlers/vm_handler_entry.h"

class ia32_movsx_handler final : public vm_handler_entry
{
public:
    ia32_movsx_handler(vm_register_manager* manager, vm_handler_generator* handler_generator)
        : vm_handler_entry(manager, handler_generator)
    {
        supported_sizes = { bit64, bit32, bit16 };
        first_operand_as_ea = true;
    }

private:
    void construct_single(function_container& container, reg_size size) override;
    encode_status encode_operand(function_container& container, const zydis_decode& instruction, zydis_dreg op_reg, int index) override;
    encode_status encode_operand(function_container& container, const zydis_decode& instruction, zydis_dmem op_mem, int index) override;

    void upscale_temp(function_container& container, reg_size target, reg_size current);
};