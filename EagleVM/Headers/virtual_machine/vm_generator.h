#pragma once
#include <map>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <intrin.h>

#include <Zydis/Zydis.h>

#include "virtual_machine/models/vm_defs.h"
#include "virtual_machine/vm_handle_generator.h"
#include "virtual_machine/vm_register_manager.h"
#include "util/math.h"
#include "util/util.h"

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
    void init_ran_consts();
    void generate_vm_handlers(uint32_t va_of_section);

    std::vector<zydis_encoder_request> translate_to_virtual(const zydis_decode& decoded_instruction);
    static std::vector<uint8_t> create_padding(size_t bytes);

private:
    vm_register_manager rm_;

    uint32_t vm_enter_va_ = 0x1;
    uint32_t vm_exit_va_ = 0x1;

    std::vector<uint8_t> section_data_;

    inline zydis_instructions create_func_jump(uint32_t address);
    encode_data encode_operand(const zydis_decode& instruction, zydis_dreg op_reg);
    encode_data encode_operand(const zydis_decode& instruction, zydis_dmem op_mem);
    encode_data encode_operand(const zydis_decode& instruction, zydis_dptr op_ptr);
    encode_data encode_operand(const zydis_decode& instruction, zydis_dimm op_imm);
};
