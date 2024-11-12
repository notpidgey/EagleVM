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
        using req = ZydisEncoderRequest;

        using op = ZydisEncoderOperand;
        using op_imm = ZydisEncoderOperand::ZydisEncoderOperandImm_;
        using op_mem = ZydisEncoderOperand::ZydisEncoderOperandMem_;
        using op_ptr = ZydisEncoderOperand::ZydisEncoderOperandPtr_;
        using op_reg = ZydisEncoderOperand::ZydisEncoderOperandReg_;
    }

    namespace dec
    {
        using inst = ZydisDecodedInstruction;

        using operand = ZydisDecodedOperand;
        using op_imm = ZydisDecodedOperandImm_;
        using op_mem = ZydisDecodedOperandMem_;
        using op_ptr = ZydisDecodedOperandPtr_;
        using op_reg = ZydisDecodedOperandReg_;

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

#define ZREG(x)			    eagle::codec::enc::op_reg{ (ZydisRegister)x, 0 }
#define ZIMMU(x)		    eagle::codec::enc::op_imm{ .u = x }
#define ZIMMS(x)		    eagle::codec::enc::op_imm{ .s = x }

// Z BYTES MEM[REG(X) + Y]
#define ZMEMBD(x, y, z)	    eagle::codec::enc::op_mem{ (ZydisRegister)x, (ZydisRegister)0, 0, (ZyanI64)y, (ZyanU16)z }

// A BYTES MEM[REG(X) + (REG(Y) * Z)]
#define ZMEMBI(x, y, z, a)	eagle::codec::enc::op_mem{ (ZydisRegister)x, (ZydisRegister)y, (ZyanU8)z, (ZyanI64)0, (ZyanU16)a }

#define RECOMPILE(...) [=](uint64_t rva) { return __VA_ARGS__ ; }
#define ZLABEL(x) ZIMMS(int32_t(x->get_relative_address()))

#define ZJMP(x, y) ZIMMU(x->get_relative_address() - y->get_relative_address())
#define ZJMPR(x) ZIMMU(x->get_relative_address() - rva)
#define ZJMPI(x) ZIMMU(x - rva)
#define TOB(x) ((uint16_t)x / 8)
