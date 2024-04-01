#include "eaglevm-core/virtual_machine/il/handlers/vm/vm_load.h"

namespace eagle::il::handlers
{
    il_insts vm_load::gen_il(reg_size size, uint8_t operands)
    {
        il_insts insts = {
            std::make_shared<cmd_handler_call>(call_type::vm_handler),
            std::make_shared<cmd_handler_call>(call_type::vm_handler),
        };

        return insts;
    }
}
