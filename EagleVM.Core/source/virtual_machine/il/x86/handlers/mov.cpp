#include "eaglevm-core/virtual_machine/il/x86/handlers/mov.h"

namespace eagle::il::handler
{
    mov::mov()
    {
        entries = {
            { codec::gpr_64, 1 },
            { codec::gpr_32, 1 },
            { codec::gpr_16, 1 },
        };
    }

    il_insts mov::gen_handler(codec::reg_class size, uint8_t operands)
    {
    }
}
