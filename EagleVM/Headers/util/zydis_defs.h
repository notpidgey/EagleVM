#pragma once
#include <Zydis/Zydis.h>
#include <Zycore/LibC.h>

//don't know how else to name this, because the other thing is also called zydis_ereg
typedef ZydisRegister zydis_register;

typedef ZydisMnemonic zyids_mnemonic;

typedef ZydisDecodedOperand_::ZydisDecodedOperandImm_ zydis_dimm;
typedef ZydisDecodedOperand_::ZydisDecodedOperandMem_ zydis_dmem;
typedef ZydisDecodedOperand_::ZydisDecodedOperandPtr_ zydis_dptr;
typedef ZydisDecodedOperand_::ZydisDecodedOperandReg_ zydis_dreg;

typedef ZydisEncoderOperand_::ZydisEncoderOperandImm_ zydis_eimm;
typedef ZydisEncoderOperand_::ZydisEncoderOperandMem_ zydis_emem;
typedef ZydisEncoderOperand_::ZydisEncoderOperandPtr_ zydis_eptr;
typedef ZydisEncoderOperand_::ZydisEncoderOperandReg_ zydis_ereg;

typedef ZydisEncoderRequest zydis_encoder_request;

typedef ZydisDecodedOperand zydis_decoded_operand;

#define ZREG(x)			{ (ZydisRegister)x, 0 }
#define ZIMMU(x)		{ .u = x }
#define ZIMMI(x)		{ .s = x }

#define ZMEMBD(x, y, z)	{ (ZydisRegister)x, (ZydisRegister)0,0, (ZyanI64)y, (ZyanU16)z }