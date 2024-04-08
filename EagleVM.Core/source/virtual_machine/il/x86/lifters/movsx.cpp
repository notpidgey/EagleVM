#include "eaglevm-core/virtual_machine/il/x86/lifters/movsx.h"

namespace eagle::il::lifter
{
    movsx::movsx()
    {
        entries = {
            { codec::gpr_64, 2 },
            { codec::gpr_32, 2 },
            { codec::gpr_16, 2 },
        };
    }

    il_insts movsx::gen_il(codec::reg_class size, uint8_t operands)
    {
    }
}
