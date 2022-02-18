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

enum register_size
{
	unsupported,
	bit64 = 8,
	bit32 = 4,
	bit16 = 2,
	bit8 = 1
};

inline ZydisDecoder zyids_decoder;

namespace zydis_helper
{
	void setup_decoder();
	zydis_register get_bit_version(zydis_register zy_register, register_size requested_size);
	register_size get_reg_size(zydis_register zy_register);

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

	template <typename T, std::same_as<T>... Rest>
	inline std::vector<T> combine_vec(const std::vector<T>& first, const std::vector<Rest>&... rest) {
		std::vector<T> result;
		result.reserve((first.size() + ... + rest.size()));
		result.insert(result.end(), first.begin(), first.end());
		(..., result.insert(result.end(), rest.begin(), rest.end()));

		return result;
	}
	
	std::vector<zydis_decode> get_instructions(void* data, size_t size);
}