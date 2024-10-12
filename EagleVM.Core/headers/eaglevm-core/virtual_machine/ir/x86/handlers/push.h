#pragma once
#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/ir/x86/base_x86_translator.h"

namespace eagle::ir::handler
{
    class push : public base_handler_gen
    {
    public:
        push();
    };
}

namespace eagle::ir::lifter
{
    class push : public base_x86_translator
    {
        using base_x86_translator::base_x86_translator;

        translate_mem_result translate_mem_result();
        void finalize_translate_to_virtual(x86_cpu_flag flags) override;
    };
}