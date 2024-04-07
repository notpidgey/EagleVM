#include "eaglevm-core/virtual_machine/il/x86/handlers/movsx.h"

namespace eagle::il::handler
{
    movsx::movsx()
    {
        entries = {
            { codec::gpr_64, 2 },
            { codec::gpr_32, 2 },
            { codec::gpr_16, 2 },
        };
    }
}
