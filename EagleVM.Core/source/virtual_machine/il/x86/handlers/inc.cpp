#include "eaglevm-core/virtual_machine/il/x86/handlers/inc.h"

#include "eaglevm-core/virtual_machine/il/commands/cmd_rflags_load.h"
#include "eaglevm-core/virtual_machine/il/commands/cmd_rflags_store.h"

namespace eagle::il::handler
{
    inc::inc()
    {
        entries = {
            { codec::gpr_64, 1 },
            { codec::gpr_32, 1 },
            { codec::gpr_16, 1 },
        };
    }

    il_insts inc::gen_handler(codec::reg_class size, uint8_t operands)
    {
    }
}

void eagle::il::lifter::inc::finalize_translate_to_virtual()
{
    block->add_command(std::make_shared<cmd_rflags_load>());
    base_x86_lifter::finalize_translate_to_virtual();
    block->add_command(std::make_shared<cmd_rflags_store>());
}
