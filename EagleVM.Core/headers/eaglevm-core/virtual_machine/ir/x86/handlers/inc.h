#pragma once
#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/ir/x86/base_x86_translator.h"

namespace eagle::ir::handler
{
    class inc : public base_handler_gen
    {
    public:
        inc();
        ir_insts gen_handler(ir_handler_sig signature) override;
    };
}

namespace eagle::ir::lifter
{
    class inc : public base_x86_translator
    {
        using base_x86_translator::base_x86_translator;

        void finalize_translate_to_virtual() override;
    };
}