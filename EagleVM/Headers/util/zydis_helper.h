#pragma once 

#include <Zydis/Zydis.h>
#include <Zycore/LibC.h>

#include <vector>
#include <iostream>

typedef ZydisEncoderOperand_::ZydisEncoderOperandImm_ ZydisImm;
typedef ZydisEncoderOperand_::ZydisEncoderOperandMem_ ZydisMem;
typedef ZydisEncoderOperand_::ZydisEncoderOperandPtr_ ZydisPtr;
typedef ZydisEncoderOperand_::ZydisEncoderOperandReg_ ZydisReg;

#define ZREG(x) {(ZydisRegister)x, 0}

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

	ZydisEncoderRequest& add_imm(ZydisEncoderRequest& req, ZydisImm imm);
	ZydisEncoderRequest& add_mem(ZydisEncoderRequest& req, ZydisMem mem);
	ZydisEncoderRequest& add_ptr(ZydisEncoderRequest& req, ZydisPtr ptr);
	ZydisEncoderRequest& add_reg(ZydisEncoderRequest& req, ZydisReg reg);
}