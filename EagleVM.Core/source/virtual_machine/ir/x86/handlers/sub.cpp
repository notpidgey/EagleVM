#include "eaglevm-core/virtual_machine/ir/x86/handlers/sub.h"

namespace eagle::il::handler
{
    sub::sub()
    {
        entries = {
            { { codec::op_none, codec::bit_8 }, { codec::op_none, codec::bit_8 } },
            { { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_16 } },
            { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_32 } },
            { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_64 } },
        };
    }

    ir_insts sub::gen_handler(codec::reg_class size, uint8_t operands)
    {
    }
}
