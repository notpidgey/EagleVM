#pragma once
#include <Zydis/Zydis.h>
#include <Zycore/LibC.h>

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

typedef std::vector<zydis_encoder_request> handle_instructions;

struct zydis_decode
{
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand		operands[ZYDIS_MAX_OPERAND_COUNT_VISIBLE];
};

#define GR_RAX  ZYDIS_REGISTER_RAX
#define GR_RCX  ZYDIS_REGISTER_RCX
#define GR_RDX  ZYDIS_REGISTER_RDX
#define GR_RBX  ZYDIS_REGISTER_RBX
#define GR_RSP  ZYDIS_REGISTER_RSP
#define GR_RBP  ZYDIS_REGISTER_RBP
#define GR_RSI  ZYDIS_REGISTER_RSI
#define GR_RDI  ZYDIS_REGISTER_RDI
#define GR_R8   ZYDIS_REGISTER_R8
#define GR_R9   ZYDIS_REGISTER_R9
#define GR_R10  ZYDIS_REGISTER_R10
#define GR_R11  ZYDIS_REGISTER_R11
#define GR_R12  ZYDIS_REGISTER_R12
#define GR_R13  ZYDIS_REGISTER_R13
#define GR_R14  ZYDIS_REGISTER_R14
#define GR_R15  ZYDIS_REGISTER_R15

#define SR_ES   ZYDIS_REGISTER_ES
#define SR_CS   ZYDIS_REGISTER_CS
#define SR_SS   ZYDIS_REGISTER_SS
#define SR_DS   ZYDIS_REGISTER_DS
#define SR_FS   ZYDIS_REGISTER_FS
#define SR_GS   ZYDIS_REGISTER_GS

#define IP_RIP  ZYDIS_REGISTER_RIP

#define ZREG(x)			    { (ZydisRegister)x, 0 }
#define ZIMMU(x)		    { .u = x }
#define ZIMMS(x)		    { .s = x }
#define ZMEMBD(x, y, z)	    { (ZydisRegister)x, (ZydisRegister)0, 0, (ZyanI64)y, (ZyanU16)z }
#define ZMEMBI(x, y, z, a)	{ (ZydisRegister)x, (ZydisRegister)y, (ZyanU8)z, (ZyanI64)0, (ZyanU16)a }