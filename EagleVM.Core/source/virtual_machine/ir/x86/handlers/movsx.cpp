#include "eaglevm-core/virtual_machine/ir/x86/handlers/movsx.h"

#include "eaglevm-core/compiler/code_label.h"
#include "eaglevm-core/virtual_machine/ir/models/ir_store.h"

namespace eagle::ir::handler
{
    movsx::movsx()
    {
        valid_operands = {
            { { { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_8 } }, "movsx, 16,8" },
            { { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_8 } }, "movsx, 32,8" },
            { { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_8 } }, "movsx, 64,8" },

            { { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_16 } }, "movsx, 32,16" },
            { { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_16 } }, "movsx, 64,16" },
        };
    }
}

namespace eagle::ir::lifter
{
    translate_status movsx::encode_operand(codec::dec::op_mem op_mem, const uint8_t idx)
    {
        base_x86_translator::encode_operand(op_mem, idx);

        const ir_size target = get_op_width();
        const ir_size size = static_cast<ir_size>(operands[idx].size);

        block->add_command(std::make_shared<cmd_sx>(target, size));
        return translate_status::success;
    }
}
