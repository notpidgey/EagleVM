#pragma once
#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/ir/x86/base_x86_translator.h"

namespace eagle::ir::handler
{
    class mov : public base_handler_gen
    {
    public:
        mov();
    };
}

namespace eagle::ir::lifter
{
    class mov : public base_x86_translator
    {
        using base_x86_translator::base_x86_translator;

        bool virtualize_as_address(codec::dec::operand operand, uint8_t idx) override;
        void finalize_translate_to_virtual() override;
    };
}