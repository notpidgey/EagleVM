#include "eaglevm-core/virtual_machine/ir/x86/handlers/pop.h"

namespace eagle::ir::handler
{
    pop::pop()
    {
        valid_operands = {
            { { { codec::op_none, codec::bit_8 } }, "pop 8" },
            { { { codec::op_none, codec::bit_16 } }, "pop 16" },
            { { { codec::op_none, codec::bit_32 } }, "pop 32" },
            { { { codec::op_none, codec::bit_64 } }, "pop 64" },
        };

        build_options = {
            { { ir_size::bit_8 }, "pop 8" },
            { { ir_size::bit_16 }, "pop 16" },
            { { ir_size::bit_32 }, "pop 32" },
            { { ir_size::bit_64 }, "pop 64" },
        };
    }

    ir_insts pop::gen_handler(codec::reg_class size, uint8_t operands)
    {
        codec::reg_size reg_size = get_reg_size(size);
        discrete_store_ptr store = discrete_store::create();
    }
}
