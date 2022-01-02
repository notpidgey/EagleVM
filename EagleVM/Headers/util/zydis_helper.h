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

	void add_op(ZydisEncoderRequest& req, ZydisImm imm);
	void add_op(ZydisEncoderRequest& req, ZydisMem mem);
	void add_op(ZydisEncoderRequest& req, ZydisPtr ptr);
	void add_op(ZydisEncoderRequest& req, ZydisReg reg);

	std::vector<uint8_t> encode_queue(std::vector<ZydisEncoderRequest>& queue);

	template<ZydisMnemonic TMnemonic, typename... TArgs>
	ZydisEncoderRequest create_encode_request(TArgs&&... args)
	{
		auto encoder = zydis_helper::create_encode_request(TMnemonic);
		(add_op(encoder, std::forward<TArgs>(args)), ...);

		return encoder;
	}
}