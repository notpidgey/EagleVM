#pragma once
#include "eaglevm-core/virtual_machine/il/handlers/base_handler_gen.h"

namespace eagle::il::handlers
{
    class vm_load : public base_handler_gen
    {
    public:
        il_insts gen_il(reg_size size, uint8_t operands) override;
    };
}