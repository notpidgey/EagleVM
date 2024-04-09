#include "eaglevm-core/virtual_machine/il/x86/handlers/mov.h"

namespace eagle::il::handler
{
    mov::mov()
    {
        entries = {
            { codec::gpr_64, 1, true },
            { codec::gpr_32, 1, true },
            { codec::gpr_16, 1, true },
        };
    }

    il_insts mov::gen_handler(codec::reg_class size, uint8_t operands)
    {
    }
}
