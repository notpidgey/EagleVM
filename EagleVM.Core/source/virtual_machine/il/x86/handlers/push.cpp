#include "eaglevm-core/virtual_machine/il/x86/handlers/push.h"

namespace eagle::il::handler
{
    push::push()
    {
        entries = {
            { codec::gpr_64, 1 },
            { codec::gpr_32, 1 },
            { codec::gpr_16, 1 },
            { codec::gpr_8, 1 },
        };
    }

    il_insts push::gen_handler(codec::reg_class size, uint8_t operands)
    {
    }
}