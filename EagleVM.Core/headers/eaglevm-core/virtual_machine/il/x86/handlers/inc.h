#pragma once
#include "eaglevm-core/virtual_machine/il/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/il/x86/base_x86_lifter.h"

namespace eagle::il::handler
{
    class inc : public base_handler_gen
    {
    public:
        inc();
        il_insts gen_handler(codec::reg_class size, uint8_t operands) override;
    };
}

namespace eagle::il::lifter
{
    class inc : public base_x86_lifter
    {
        using base_x86_lifter::base_x86_lifter;

        void finalize_translate_to_virtual() override;
    };
}