#pragma once
#include <map>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <intrin.h>

#include <Zydis/Zydis.h>

#include "virtual_machine/models/vm_defs.h"
#include "handlers/vm_handler_generator.h"
#include "virtual_machine/vm_register_manager.h"

#include "util/section/section_manager.h"
#include "util/util.h"

#define VIP         rm_->reg_map[I_VIP]
#define VSP         rm_->reg_map[I_VSP]
#define VREGS       rm_->reg_map[I_VREGS]
#define VTEMP       rm_->reg_map[I_VTEMP]
#define PUSHORDER   rm_->reg_stack_order_

enum class encode_status
{
    success,
    unsupported
};

typedef std::pair<function_container, encode_status> encode_data;
typedef std::tuple<uint32_t, uint32_t, instructions_vec, encoded_vec> encode_handler_data;

class base_instruction_virtualizer
{
public:
    virtual std::pair<bool, function_container> translate_to_virtual(const zydis_decode& decoded_instruction);

protected:
    explicit base_instruction_virtualizer(vm_register_manager* manager, vm_handler_generator* handler_generator);

    vm_register_manager* rm_;
    vm_handler_generator* hg_;

    virtual void create_vm_jump(function_container& container, code_label* jump_label);
    virtual encode_status encode_operand(function_container& container, const zydis_decode& instruction, zydis_dreg op_reg);
    virtual encode_status encode_operand(function_container& container, const zydis_decode& instruction, zydis_dmem op_mem);
    virtual encode_status encode_operand(function_container& container, const zydis_decode& instruction, zydis_dptr op_ptr);
    virtual encode_status encode_operand(function_container& container, const zydis_decode& instruction, zydis_dimm op_imm);
};