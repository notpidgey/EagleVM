#include "eaglevm-core/virtual_machine/il/x86/handlers/cmp.h"

namespace eagle::il::handler
{
    cmp::cmp()
    {
        entries = {
            {codec::gpr_64, 2},
            {codec::gpr_32, 2},
            {codec::gpr_16, 2},
            {codec::gpr_8, 2}
        };
    }

    il_insts cmp::gen_il(codec::reg_class size, uint8_t operands)
    {
    }
}
