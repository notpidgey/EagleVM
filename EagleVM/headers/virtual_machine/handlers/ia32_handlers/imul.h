#pragma once
#include "virtual_machine/handlers/vm_handler_entry.h"

class ia32_imul_handler final : public vm_handler_entry
{
public:
    ia32_imul_handler(vm_register_manager* manager, vm_handler_generator* handler_generator)
        : vm_handler_entry(manager, handler_generator)
    {
        supported_sizes = { bit64, bit32, bit16 };
        first_operand_as_ea = false;
    }

private:
    void construct_single(function_container& container, reg_size size) override;
    void finalize_translate_to_virtual(const zydis_decode& decoded_instruction, function_container& container) override;
};