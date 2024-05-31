#pragma once
#include <cstdint>

#include "eaglevm-core/codec/zydis_defs.h"

namespace eagle::ir
{
    struct handler_op
    {
        handler_op(const codec::op_type type, const codec::reg_size size)
        {
            operand_type = type;
            operand_size = size;
        }

        codec::op_type operand_type;
        codec::reg_size operand_size;
    };

}
