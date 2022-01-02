#pragma once 

#include <Zydis/Zydis.h>
#include <Zycore/LibC.h>

#include <vector>
#include <iostream>
#include <algorithm>

typedef ZydisEncoderOperand_::ZydisEncoderOperandImm_ ZydisImm;
typedef ZydisEncoderOperand_::ZydisEncoderOperandMem_ ZydisMem;
typedef ZydisEncoderOperand_::ZydisEncoderOperandPtr_ ZydisPtr;
typedef ZydisEncoderOperand_::ZydisEncoderOperandReg_ ZydisReg;

#define ZREG(x)			{ (ZydisRegister)x, 0 }

#define ZIMMU(x)		{ .u = x }
#define ZIMMI(x)		{ .s = x }

#define ZMEMBD(x, y, z)	{ (ZydisRegister)x, (ZydisRegister)0,0, (ZyanI64)y, z }

namespace zydis_helper
{
	template <typename T> 
	T create_new_operand()
	{
		T operand;
		ZYAN_MEMSET(&operand, 0, sizeof(operand));

		return operand;
	}

	std::vector<uint8_t> encode_request(ZydisEncoderRequest& request);
	ZydisEncoderRequest create_encode_request(ZydisMnemonic mnemonic);

	void add_imm(ZydisEncoderRequest& req, ZydisImm imm);
	void add_mem(ZydisEncoderRequest& req, ZydisMem mem);
	void add_ptr(ZydisEncoderRequest& req, ZydisPtr ptr);
	void add_reg(ZydisEncoderRequest& req, ZydisReg reg);

	std::vector<uint8_t> encode_queue(std::vector<ZydisEncoderRequest>& queue);

	template<typename T>
	void add_operand(ZydisEncoderRequest& encoder, T* operand)
	{
		if (std::is_same<T, ZydisImm>::value)
			add_imm(encoder, *(ZydisImm*)operand);
		else if (std::is_same<T, ZydisMem>::value)
			add_mem(encoder, *(ZydisMem*)operand);
		else if (std::is_same<T, ZydisPtr>::value)
			add_ptr(encoder, *(ZydisPtr*)operand);
		else if (std::is_same<T, ZydisReg>::value)
			add_reg(encoder, *(ZydisReg*)operand);
	}

	template<typename T>
	ZydisEncoderRequest create_encode_request(ZydisMnemonic mnemonic, T operand1)
	{
		auto encoder = zydis_helper::create_encode_request(mnemonic);

		//I feel terrible for having to write this, but it had to be done
		add_operand<T>(encoder, &operand1);

		return encoder;
	}

	template<typename T, typename G>
	ZydisEncoderRequest create_encode_request(ZydisMnemonic mnemonic, T operand1, G operand2)
	{
		auto encoder = zydis_helper::create_encode_request(mnemonic);

		//I feel terrible for having to write this, but it had to be done
		add_operand<T>(encoder, &operand1);
		add_operand<G>(encoder, &operand2);

		return encoder;
	}
}