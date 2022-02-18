#pragma once
#include <map>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <intrin.h>

#include <Zydis/Zydis.h>

#include "virtual_machine/models/vm_defs.h"
#include "virtual_machine/handle_generator.h"
#include "virtual_machine/vm_register_manager.h"
#include "util/math.h"

enum class encode_status
{
    success,
    unsupported
};

typedef std::vector<zydis_encoder_request> zydis_instructions;
typedef std::pair<zydis_instructions, encode_status> encode_data;

class vm_generator
{
public:
    vm_handle_generator hg_;

    vm_generator();

    void init_reg_order();
    void init_vreg_map();
    void init_ran_consts();

    std::vector<zydis_encoder_request> translate_to_virtual(const zydis_decode& decoded_instruction);

    std::vector<uint8_t> create_padding(size_t bytes);

    std::vector<uint8_t> create_vm_enter_jump(uint32_t va_protected);
    std::vector<uint8_t> create_vm_enter();

private:
    vm_register_manager rm_;
	
	uint32_t vm_enter_va_ = 0x1;
    uint32_t vm_exit_va_ = 0x1;

    std::vector<char> section_data_;

    union jmp_enc_constants
    {
        uint8_t values[3] = {};
    } func_address_keys_;

    encode_data encode_register_operand(zydis_dreg op_reg);
    encode_data encode_memory_operand(zydis_dmem op_mem);
    encode_data encode_pointer_operand(zydis_dptr op_ptr);
    encode_data encode_immediate_operand(zydis_dimm op_imm);
};
