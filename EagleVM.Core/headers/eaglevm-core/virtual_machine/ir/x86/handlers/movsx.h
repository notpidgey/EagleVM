#pragma once
#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/ir/x86/base_x86_translator.h"

namespace eagle::ir::handler
{
    class movsx : public base_handler_gen
    {
    public:
        movsx();
        ir_insts gen_handler(codec::reg_class size, uint8_t operands) override;
    };
}

namespace eagle::ir::lifter
{
    class movsx : public base_x86_translator
    {
        using base_x86_translator::base_x86_translator;

        translate_status encode_operand(codec::dec::op_mem op_mem, uint8_t idx) override;
    };
}