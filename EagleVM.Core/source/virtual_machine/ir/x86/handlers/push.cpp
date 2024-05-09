#include "eaglevm-core/virtual_machine/ir/x86/handlers/push.h"

namespace eagle::il::handler
{
    push::push()
    {
        entries = {
            { { codec::op_none, codec::bit_8 } },
            { { codec::op_none, codec::bit_16 } },
            { { codec::op_none, codec::bit_32 } },
            { { codec::op_none, codec::bit_64 } }
        };
    }

    il_insts push::gen_handler(codec::reg_class size, uint8_t operands)
    {
    }
}
