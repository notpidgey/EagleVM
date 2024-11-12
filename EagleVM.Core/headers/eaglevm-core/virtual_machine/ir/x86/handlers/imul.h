#pragma once
#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/ir/x86/base_x86_translator.h"

namespace eagle::ir::handler
{
    class imul : public base_handler_gen
    {
    public:
        imul();
        ir_insts gen_handler(handler_sig signature) override;

    private:
        ir_insts compute_of_cf(ir_size size);
    };
}

namespace eagle::ir::lifter
{
    class imul : public base_x86_translator
    {
        using base_x86_translator::base_x86_translator;

        translate_mem_result translate_mem_action(const codec::dec::op_mem& op_mem, uint8_t idx) override;
        void finalize_translate_to_virtual(x86_cpu_flag flags) override;
        bool skip(uint8_t idx) override;
    };
}