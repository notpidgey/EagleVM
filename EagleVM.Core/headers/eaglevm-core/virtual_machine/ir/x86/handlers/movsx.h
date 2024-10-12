#pragma once
#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/ir/x86/base_x86_translator.h"

namespace eagle::ir::handler
{
    class movsx : public base_handler_gen
    {
    public:
        movsx();
    };
}

namespace eagle::ir::lifter
{
    class movsx : public base_x86_translator
    {
        using base_x86_translator::base_x86_translator;

        translate_status encode_operand(codec::dec::op_mem op_mem, uint8_t idx) override;
        translate_status encode_operand(codec::dec::op_reg op_reg, uint8_t idx) override;

        translate_mem_result translate_mem_action(const codec::dec::op_mem& op_mem, uint8_t idx);
        void finalize_translate_to_virtual(x86_cpu_flag flags);
        bool skip(uint8_t idx);
    };
}