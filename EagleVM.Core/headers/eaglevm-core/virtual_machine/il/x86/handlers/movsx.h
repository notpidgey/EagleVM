#pragma once
#include "eaglevm-core/virtual_machine/il/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/il/x86/base_x86_lifter.h"

namespace eagle::il::handler
{
    class movsx : public base_handler_gen
    {
    public:
        movsx();
        il_insts gen_handler(codec::reg_class size, uint8_t operands) override;
    };
}

namespace eagle::il::lifter
{
    class movsx : public base_x86_lifter
    {
        using base_x86_lifter::base_x86_lifter;

        translate_status encode_operand(codec::dec::op_mem op_mem, uint8_t idx) override;
    };
}