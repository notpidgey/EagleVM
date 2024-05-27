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
}

namespace eagle::ir::lifter
{
    bool pop::virtualize_as_address(codec::dec::operand operand, uint8_t idx)
    {
        // we only want to address of a memory operand
        return operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY;
    }

    bool pop::skip(uint8_t idx)
    {
        return operands[0].type != ZYDIS_OPERAND_TYPE_MEMORY;
    }

    void pop::finalize_translate_to_virtual()
    {
        ir_size size = get_op_width();

        discrete_store_ptr store = discrete_store::create(size);
        block->add_command(std::make_shared<cmd_pop>(store, size));

        auto first_op = operands[0];
        switch (first_op.type)
        {
            case ZYDIS_OPERAND_TYPE_MEMORY:
            {
                block->add_command(std::make_shared<cmd_mem_write>(size, size));
                break;
            }
            case ZYDIS_OPERAND_TYPE_REGISTER:
            {
                codec::reg target_reg = static_cast<codec::reg>(first_op.reg.value);
                block->add_command(std::make_shared<cmd_context_store>(target_reg));

                break;
            }
        }
    }
}
