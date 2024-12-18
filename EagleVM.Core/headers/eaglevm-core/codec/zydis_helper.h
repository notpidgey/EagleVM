#pragma once

#include <Zydis/Zydis.h>
#include <Zycore/LibC.h>

#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/codec/zydis_enum.h"

#define TO8(x) zydis_helper::get_bit_version(x, reg_size::bit8)
#define TO16(x) zydis_helper::get_bit_version(x, reg_size::bit16)
#define TO32(x) zydis_helper::get_bit_version(x, reg_size::bit32)
#define TO64(x) zydis_helper::get_bit_version(x, reg_size::bit64)

inline ZydisDecoder zyids_decoder;
inline ZydisFormatter zydis_formatter;

namespace eagle::codec
{
    void setup_decoder();

    reg get_bit_version(reg input_reg, reg_size target_size);
    reg get_bit_version(reg input_reg, reg_class target_size);
    reg get_bit_version(zydis_register input_reg, reg_class target_size);
    reg get_largest_enclosing(reg input_reg);

    uint16_t reg_size_to_b(reg_size size);
    bool is_upper_8(reg reg);

    reg_class get_class_from_size(reg_size size);

    reg_class get_reg_class(reg reg);
    reg_class get_reg_class(zydis_register reg);

    reg_size get_reg_size(reg reg);
    reg_size get_reg_size(zydis_register reg);
    reg_size get_reg_size(reg_class reg);

    char reg_size_to_string(reg_class reg_size);

    std::vector<uint8_t> encode_request(const enc::req& request);
    enc::req create_encode_request(mnemonic mnemonic);
    enc::req decode_to_encode(const dec::inst_info& decode);

    void add_op(enc::req& req, enc::op_imm imm);
    void add_op(enc::req& req, enc::op_mem mem);
    void add_op(enc::req& req, enc::op_ptr ptr);
    void add_op(enc::req& req, enc::op_reg reg);

    std::string instruction_to_string(const dec::inst_info& decode);
    std::string operand_to_string(const dec::inst_info& decode, int index);
    const char* reg_to_string(reg reg);

    std::vector<uint8_t> compile(enc::req& request);
    std::vector<uint8_t> compile_absolute(enc::req& request, uint32_t address);

    std::vector<uint8_t> compile_queue(std::vector<enc::req>& queue);
    std::vector<uint8_t> compile_queue_absolute(std::vector<enc::req>& queue);

    std::vector<std::string> print(enc::req& queue, uint32_t address);
    std::vector<std::string> print_queue(const std::vector<enc::req>& queue, uint32_t address);

    bool contains_rip_relative_operand(dec::inst_info& decode);
    bool is_jmp_or_jcc(mnemonic mnemonic);
    std::pair<uint64_t, uint8_t> calc_relative_rva(const dec::inst& instruction, const dec::operand* operands, uint32_t rva, int8_t operand = -1);
    std::pair<uint64_t, uint8_t> calc_relative_rva(const dec::inst_info& decode, uint32_t rva, int8_t operand = -1);

    enc::req encode(const mnemonic mnemonic, auto&&... args)
    {
        auto encoder = create_encode_request(mnemonic);
        (add_op(encoder, std::forward<decltype(args)>(args)), ...);

        return encoder;
    }

    std::vector<dec::inst_info> get_instructions(void* data, size_t size);
    dec::inst_info get_instruction(void* data, size_t size, uint64_t offset);
}
