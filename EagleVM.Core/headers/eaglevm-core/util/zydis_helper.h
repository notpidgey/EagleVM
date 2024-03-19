#pragma once

#include <Zydis/Zydis.h>
#include <Zycore/LibC.h>

#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include "eaglevm-core/util/zydis_defs.h"

#define TO8(x) zydis_helper::get_bit_version(x, reg_size::bit8)
#define TO16(x) zydis_helper::get_bit_version(x, reg_size::bit16)
#define TO32(x) zydis_helper::get_bit_version(x, reg_size::bit32)
#define TO64(x) zydis_helper::get_bit_version(x, reg_size::bit64)

enum reg_size
{
	unsupported,
	bit64 = 8,
	bit32 = 4,
	bit16 = 2,
	bit8 = 1
};

inline ZydisDecoder zyids_decoder;
inline ZydisFormatter zydis_formatter;

namespace zydis_helper
{
	void setup_decoder();
	zydis_register get_bit_version(zydis_register zy_register, reg_size requested_size);
	bool is_upper_8(zydis_register zy_register);
	reg_size get_reg_size(zydis_register zy_register);
    char reg_size_to_string(reg_size reg_size);

	/// Zydis Encoder Helpers
	std::vector<uint8_t> encode_request(zydis_encoder_request& request);
	zydis_encoder_request create_encode_request(zyids_mnemonic mnemonic);

	zydis_encoder_request decode_to_encode(const zydis_decode& decode);

	void add_op(zydis_encoder_request& req, zydis_eimm imm);
	void add_op(zydis_encoder_request& req, zydis_emem mem);
	void add_op(zydis_encoder_request& req, zydis_eptr ptr);
	void add_op(zydis_encoder_request& req, zydis_ereg reg);

	std::string instruction_to_string(const zydis_decode& decode);
	std::string operand_to_string(const zydis_decode& decode, int index);

	std::vector<uint8_t> compile(zydis_encoder_request& request);
	std::vector<uint8_t> compile_absolute(zydis_encoder_request& request, uint32_t address);

	std::vector<uint8_t> compile_queue(std::vector<zydis_encoder_request>& queue);
    std::vector<uint8_t> compile_queue_absolute(std::vector<zydis_encoder_request>& queue);

    std::vector<std::string> print(zydis_encoder_request& queue, uint32_t address);
    std::vector<std::string> print_queue(std::vector<zydis_encoder_request>& queue, uint32_t address);

	bool has_relative_operand(zydis_decode& decode);
	std::pair<uint64_t, uint8_t> calc_relative_rva(const zydis_decode& decode, uint32_t rva, int8_t operand = -1);
	
	zydis_encoder_request enc(zyids_mnemonic mnemonic, auto&&... args)
	{
		auto encoder = create_encode_request(mnemonic);
		(add_op(encoder, std::forward<decltype(args)>(args)), ...);

		return encoder;
	}

	std::vector<zydis_decode> get_instructions(void* data, size_t size);
}