#include "eaglevm-core/virtual_machine/il/x86/lifters/push.h"

namespace eagle::il::lifter
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

    il_insts push::gen_il(codec::reg_class size, uint8_t operands)
    {
    }
}
