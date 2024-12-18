#pragma once
#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/ir/x86/base_x86_translator.h"

namespace eagle::ir::handler
{
    class ret : public base_handler_gen
    {
    public:
        ret();
        ir_insts gen_handler(handler_sig signature) override;
    };
}

namespace eagle::ir::lifter
{
    class ret : public base_x86_translator
    {
        using base_x86_translator::base_x86_translator;
        bool translate_to_il(uint64_t original_rva, x86_cpu_flag flags = NONE) override;
    };
}