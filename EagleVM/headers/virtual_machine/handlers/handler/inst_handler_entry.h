#pragma once
#include "virtual_machine/handlers/handler/base_handler_entry.h"

class inst_handler_entry : public base_handler_entry
{
public:
    inst_handler_entry(vm_register_manager* manager, vm_handler_generator* handler_generator)
        : base_handler_entry(manager, handler_generator) { }

    virtual std::pair<bool, function_container> translate_to_virtual(const zydis_decode& decoded_instruction);
    code_label* get_handler_va(reg_size width, uint8_t operands) const;

protected:
    virtual encode_status encode_operand(function_container& container, const zydis_decode& instruction, zydis_dreg op_reg, int& stack_disp, int index);
    virtual encode_status encode_operand(function_container& container, const zydis_decode& instruction, zydis_dmem op_mem, int& stack_disp, int index);
    virtual encode_status encode_operand(function_container& container, const zydis_decode& instruction, zydis_dptr op_ptr, int& stack_disp);
    virtual encode_status encode_operand(function_container& container, const zydis_decode& instruction, zydis_dimm op_imm, int& stack_disp);

    virtual void finalize_translate_to_virtual(const zydis_decode& decoded_instruction, function_container& container);
};