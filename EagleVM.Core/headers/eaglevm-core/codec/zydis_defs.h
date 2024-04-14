#pragma once
#include <functional>
#include <variant>

#include <Zydis/Zydis.h>
#include <Zydis/DecoderTypes.h>
#include <Zycore/LibC.h>

#include "eaglevm-core/codec/zydis_enum.h"

namespace eagle::codec
{
    namespace enc
    {
        typedef ZydisEncoderRequest req;

        typedef ZydisEncoderOperand_::ZydisEncoderOperandImm_ op_imm;
        typedef ZydisEncoderOperand_::ZydisEncoderOperandMem_ op_mem;
        typedef ZydisEncoderOperand_::ZydisEncoderOperandPtr_ op_ptr;
        typedef ZydisEncoderOperand_::ZydisEncoderOperandReg_ op_reg;
    }

    namespace dec
    {
        typedef ZydisDecodedInstruction inst;

        typedef ZydisDecodedOperand operand;
        typedef ZydisDecodedOperandImm_ op_imm;
        typedef ZydisDecodedOperandMem_ op_mem;
        typedef ZydisDecodedOperandPtr_ op_ptr;
        typedef ZydisDecodedOperandReg_ op_reg;

        struct inst_info
        {
            inst instruction;
            operand operands[ZYDIS_MAX_OPERAND_COUNT];
        };
    }

    typedef std::function<enc::req(uint64_t)> recompile_promise;
    typedef std::variant<enc::req, recompile_promise> dynamic_instruction;

    typedef std::vector<dynamic_instruction> dynamic_instructions_vec;
    typedef std::vector<enc::req> instructions_vec;
    typedef std::vector<uint8_t> encoded_vec;

    typedef std::vector<dec::inst_info> decode_vec;
}

#define ZREG(x)			    eagle::asmbl::x86::op_reg{ (ZydisRegister)x, 0 }
#define ZIMMU(x)		    eagle::asmbl::x86::op_imm{ .u = x }
#define ZIMMS(x)		    eagle::asmbl::x86::op_imm{ .s = x }

// Z BYTES MEM[REG(X) + Y]
#define ZMEMBD(x, y, z)	    eagle::asmbl::x86::op_mem{ (ZydisRegister)x, (ZydisRegister)0, 0, (ZyanI64)y, (ZyanU16)z }

// A BYTES MEM[REG(X) + (REG(Y) * Z)]
#define ZMEMBI(x, y, z, a)	eagle::asmbl::x86::op_mem{ (ZydisRegister)x, (ZydisRegister)y, (ZyanU8)z, (ZyanI64)0, (ZyanU16)a }

#define RECOMPILE(...) [=](uint64_t rva) { return __VA_ARGS__ ; }
#define ZLABEL(x) ZIMMS(int32_t(x->get()))
#define ZJMP(x, y) ZIMMS(x->get() - y->get())
