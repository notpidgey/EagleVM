#pragma once
#include <vector>
#include "eaglevm-core/codec/zydis_enum.h"

namespace eagle::ir
{
    struct x86_operand
    {
        codec::op_type op_type;
        codec::reg_size reg_size;
    };

    using x86_operand_sig = std::vector<x86_operand>;
}
