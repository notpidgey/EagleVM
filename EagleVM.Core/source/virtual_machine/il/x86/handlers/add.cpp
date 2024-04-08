#include "eaglevm-core/virtual_machine/il/x86/handlers/add.h"

namespace eagle::il::handler
{
    add::add()
    {
        entries = {
            {codec::gpr_64, 2},
            {codec::gpr_32, 2},
            {codec::gpr_16, 2},
        };
    }

    il_insts add::gen_handler(const codec::reg_class size, uint8_t operands)
    {
        const il_size target_size = static_cast<il_size>(get_reg_size(size));
        return {
            std::make_shared<cmd_vm_pop>(reg_vm::vtemp, target_size)
        };
    }
}
