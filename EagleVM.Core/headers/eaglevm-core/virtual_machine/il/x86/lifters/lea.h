#pragma once
#include "eaglevm-core/virtual_machine/il/x86/base_handler_gen.h"

namespace eagle::il::lifter
{
    class lea : public base_handler_gen
    {
    public:
        lea();
        il_insts gen_il(codec::reg_class size, uint8_t operands) override;
    };
}