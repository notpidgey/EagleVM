#include "eaglevm-core/virtual_machine/il/x86/handlers/pop.h"

namespace eagle::il::handler
{
    pop::pop()
    {
        entries = {
            { { codec::op_none, codec::bit_8 } },
            { { codec::op_none, codec::bit_16 } },
            { { codec::op_none, codec::bit_32 } },
            { { codec::op_none, codec::bit_64 } }
        };
    }

    il_insts pop::gen_handler(codec::reg_class size, uint8_t operands)
    {
    }
}
