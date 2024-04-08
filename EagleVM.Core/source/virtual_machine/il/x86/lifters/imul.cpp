#include "eaglevm-core/virtual_machine/il/x86/lifters/imul.h"

namespace eagle::il::lifter
{
    imul::imul()
    {
        entries = {
            { codec::gpr_64, 2 },
            { codec::gpr_32, 2 },
            { codec::gpr_16, 2 },

            // its 3 operands but we handle in finalize_translate_to_virtual
            { codec::gpr_64, 3 },
            { codec::gpr_32, 3 },
            { codec::gpr_16, 3 },
        };
    }

    il_insts imul::gen_il(codec::reg_class size, uint8_t operands)
    {
    }
}
