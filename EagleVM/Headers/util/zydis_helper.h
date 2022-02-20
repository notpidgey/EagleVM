#pragma once 

#include <Zydis/Zydis.h>
#include <Zycore/LibC.h>

#include <vector>
#include <iostream>
#include <algorithm>

#include "util/zydis_defs.h"

struct zydis_decode
{
	ZydisDecodedInstruction instruction;
	ZydisDecodedOperand		operands[ZYDIS_MAX_OPERAND_COUNT_VISIBLE];
};

enum reg_size
{
	unsupported,
	bit64 = 8,
	bit32 = 4,
	bit16 = 2,
	bit8 = 1
};

inline ZydisDecoder zyids_decoder;
typedef std::vector<zydis_encoder_request> handle_instructions;

namespace zydis_helper
{
	void setup_decoder();
	zydis_register get_bit_version(zydis_register zy_register, reg_size requested_size);
	reg_size get_reg_size(zydis_register zy_register);
    char reg_size_to_string(reg_size reg_size);

	/// Zydis Encoder Helpers
	std::vector<uint8_t> encode_request(zydis_encoder_request& request);
	zydis_encoder_request create_encode_request(zyids_mnemonic mnemonic);

	void add_op(zydis_encoder_request& req, zydis_eimm imm);
	void add_op(zydis_encoder_request& req, zydis_emem mem);
	void add_op(zydis_encoder_request& req, zydis_eptr ptr);
	void add_op(zydis_encoder_request& req, zydis_ereg reg);

	std::vector<uint8_t> encode_queue(std::vector<zydis_encoder_request>& queue);

	template<zyids_mnemonic TMnemonic, typename... TArgs>
	zydis_encoder_request create_encode_request(TArgs&&... args)
	{
		auto encoder = zydis_helper::create_encode_request(TMnemonic);
		(add_op(encoder, std::forward<TArgs>(args)), ...);

		return encoder;
	}
	
	std::vector<zydis_decode> get_instructions(void* data, size_t size);
}