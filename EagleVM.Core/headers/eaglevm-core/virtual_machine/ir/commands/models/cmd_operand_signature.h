#pragma once
#include <vector>
#include "eaglevm-core/codec/zydis_enum.h"

namespace eagle::ir
{
    struct x86_operand
    {
        x86_operand(const codec::op_type type, const codec::reg_size size)
        {
            operand_type = type;
            operand_size = size;
        }

        codec::op_type operand_type;
        codec::reg_size operand_size;

        bool operator==(const x86_operand& other) const
        {
            return operand_type == other.operand_type && operand_size == other.operand_size;
        }
    };

    using x86_operand_sig = std::vector<x86_operand>;
}
