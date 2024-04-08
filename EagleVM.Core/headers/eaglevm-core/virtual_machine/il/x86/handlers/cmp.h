#pragma once
#include "eaglevm-core/virtual_machine/il/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/il/x86/base_x86_lifter.h"

namespace eagle::il::handler
{
    class cmp : public base_handler_gen
    {
    public:
        cmp();
        il_insts gen_handler(codec::reg_class size, uint8_t operands) override;
    };
}

namespace eagle::il::lifter
{
    class cmp : public base_x86_lifter
    {
        translate_status encode_operand(codec::dec::op_mem op_mem, uint8_t idx) override;
    };
}