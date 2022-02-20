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

#define SR_RAX ZYDIS_REGISTER_RAX
#define SR_RCX ZYDIS_REGISTER_RCX
#define SR_RDX ZYDIS_REGISTER_RDX
#define SR_RBX ZYDIS_REGISTER_RBX
#define SR_RSP ZYDIS_REGISTER_RSP
#define SR_RBP ZYDIS_REGISTER_RBP
#define SR_RSI ZYDIS_REGISTER_RSI
#define SR_RDI ZYDIS_REGISTER_RDI
#define SR_R8  ZYDIS_REGISTER_R8
#define SR_R9  ZYDIS_REGISTER_R9
#define SR_R10 ZYDIS_REGISTER_R10
#define SR_R11 ZYDIS_REGISTER_R11
#define SR_R12 ZYDIS_REGISTER_R12
#define SR_R13 ZYDIS_REGISTER_R13
#define SR_R14 ZYDIS_REGISTER_R14
#define SR_R15 ZYDIS_REGISTER_R15

#define ZREG(x)			    { (ZydisRegister)x, 0 }
#define ZIMMU(x)		    { .u = x }
#define ZIMMI(x)		    { .s = x }
#define ZMEMBD(x, y, z)	    { (ZydisRegister)x, (ZydisRegister)0, 0, (ZyanI64)y, (ZyanU16)z }
#define ZMEMBI(x, y, z, a)	{ (ZydisRegister)x, (ZydisRegister)y, (ZyanU8)z, (ZyanI64)0, (ZyanU16)a }