#include "eaglevm-core/virtual_machine/ir/x86/handlers/push.h"

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

    ir_insts push::gen_handler(codec::reg_class size, uint8_t operands)
    {
    }
}
