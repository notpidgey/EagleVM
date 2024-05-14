#pragma once
#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/ir/x86/base_x86_translator.h"

namespace eagle::ir::handler
{
    class mov : public base_handler_gen
    {
    public:
        mov();
        ir_insts gen_handler(codec::reg_class size, uint8_t operands) override;
    };
}

namespace eagle::ir::lifter
{
    class mov : public base_x86_translator
    {
        using base_x86_translator::base_x86_translator;
    };
}