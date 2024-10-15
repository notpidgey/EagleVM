#pragma once
#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/ir/x86/base_x86_translator.h"

namespace eagle::ir::handler
{
    class inc : public base_handler_gen
    {
    public:
        inc();
        ir_insts gen_handler(handler_sig signature) override;

    private:
        ir_insts compute_of(ir_size size, const discrete_store_ptr& result, const discrete_store_ptr& value, const discrete_store_ptr& flags);
        ir_insts compute_af(ir_size size, const discrete_store_ptr& result, const discrete_store_ptr& value, const discrete_store_ptr& flags);
    };
}

namespace eagle::ir::lifter
{
    class inc : public base_x86_translator
    {
        using base_x86_translator::base_x86_translator;

        translate_mem_result translate_mem_action(const codec::dec::op_mem& op_mem, uint8_t idx) override;
        void finalize_translate_to_virtual(x86_cpu_flag flags) override;
    };
}