#include "eaglevm-core/virtual_machine/ir/x86/handlers/push.h"

#include "eaglevm-core/virtual_machine/ir/block_builder.h"

namespace eagle::ir::handler
{
    push::push()
    {
        valid_operands = {
            { { { codec::op_none, codec::bit_8 } }, "push 8" },
            { { { codec::op_none, codec::bit_16 } }, "push 16" },
            { { { codec::op_none, codec::bit_32 } }, "push 32" },
            { { { codec::op_none, codec::bit_64 } }, "push 64" },
        };

        build_options = {
            { { ir_size::bit_8 }, "push 8" },
            { { ir_size::bit_16 }, "push 16" },
            { { ir_size::bit_32 }, "push 32" },
            { { ir_size::bit_64 }, "push 64" },
        };
    }
}

namespace eagle::ir::lifter
{
    translate_mem_result push::translate_mem_result()
    {
        return translate_mem_result::value;
    }

    void push::finalize_translate_to_virtual(x86_cpu_flag flags)
    {
        // dont do anything because the operand encoders automatically push values
        // base_x86_translator::finalize_translate_to_virtual();
    }
}

