#pragma once 

#include <Zydis/Zydis.h>
#include <Zycore/LibC.h>

#include <vector>
#include <iostream>

typedef ZydisEncoderOperand_::ZydisEncoderOperandImm_ ZydisImm;
typedef ZydisEncoderOperand_::ZydisEncoderOperandMem_ ZydisMem;
typedef ZydisEncoderOperand_::ZydisEncoderOperandPtr_ ZydisPtr;
typedef ZydisEncoderOperand_::ZydisEncoderOperandReg_ ZydisReg;

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
	ZydisEncoderRequest create_encode_request(ZydisMnemonic mnemonic, int operand_count);
	
	std::vector<uint8_t> encode_instruction_0(ZydisMnemonic mnemonic);

	std::vector<uint8_t> encode_instruction_1(ZydisMnemonic mnemonic, ZydisImm imm);
	std::vector<uint8_t> encode_instruction_1(ZydisMnemonic mnemonic, ZydisMem mem);
	std::vector<uint8_t> encode_instruction_1(ZydisMnemonic mnemonic, ZydisPtr ptr);
	std::vector<uint8_t> encode_instruction_1(ZydisMnemonic mnemonic, ZydisReg reg);
}