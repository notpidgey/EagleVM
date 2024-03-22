#pragma once
#include <Zydis/Zydis.h>
#include <Zydis/DecoderTypes.h>
#include <Zycore/LibC.h>

typedef ZydisRegister zydis_register;
typedef ZydisMnemonic zyids_mnemonic;

typedef ZydisOperandType zyids_operand_t;
typedef ZydisDecodedOperandImm_ zydis_dimm;
typedef ZydisDecodedOperandMem_ zydis_dmem;
typedef ZydisDecodedOperandPtr_ zydis_dptr;
typedef ZydisDecodedOperandReg_ zydis_dreg;

typedef ZydisEncoderOperand_::ZydisEncoderOperandImm_ zydis_eimm;
typedef ZydisEncoderOperand_::ZydisEncoderOperandMem_ zydis_emem;
typedef ZydisEncoderOperand_::ZydisEncoderOperandPtr_ zydis_eptr;
typedef ZydisEncoderOperand_::ZydisEncoderOperandReg_ zydis_ereg;

typedef ZydisEncoderRequest zydis_encoder_request;
typedef ZydisDecodedInstruction zydis_decoded_inst;
typedef ZydisDecodedOperand zydis_decoded_operand;

#include <functional>
#include <variant>

typedef std::function<zydis_encoder_request(uint64_t)> recompile_promise;
typedef std::variant<zydis_encoder_request, recompile_promise> dynamic_instruction;

typedef std::vector<dynamic_instruction> dynamic_instructions_vec;
typedef std::vector<zydis_encoder_request> instructions_vec;
typedef std::vector<uint8_t> encoded_vec;

#define RECOMPILE(...) [=](uint64_t rva) { return __VA_ARGS__ ; }

#define ZLABEL(x) ZIMMS(int32_t(x->get()))

#define ZJMP(x, y) ZIMMS(x->get() - y->get())

struct zydis_decode
{
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand		operands[ZYDIS_MAX_OPERAND_COUNT];
};
typedef std::vector<zydis_decode> decode_vec;

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

#define ZREG(x)			    zydis_ereg{ (ZydisRegister)x, 0 }
#define ZIMMU(x)		    zydis_eimm{ .u = x }
#define ZIMMS(x)		    zydis_eimm{ .s = x }

// Z BYTES MEM[REG(X) + Y]
#define ZMEMBD(x, y, z)	    zydis_emem{ (ZydisRegister)x, (ZydisRegister)0, 0, (ZyanI64)y, (ZyanU16)z }

// A BYTES MEM[REG(X) + (REG(Y) * Z)]
#define ZMEMBI(x, y, z, a)	zydis_emem{ (ZydisRegister)x, (ZydisRegister)y, (ZyanU8)z, (ZyanI64)0, (ZyanU16)a }