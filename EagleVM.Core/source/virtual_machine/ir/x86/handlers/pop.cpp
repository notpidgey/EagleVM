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
    translate_mem_result pop::translate_mem_action(const codec::dec::op_mem& op_mem, uint8_t idx)
    {
        return translate_mem_result::address;
    }

    bool pop::skip(uint8_t idx)
    {
        return operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER;
    }

    void pop::finalize_translate_to_virtual(x86_cpu_flag flags)
    {
        ir_size size = get_op_width();

        discrete_store_ptr store = discrete_store::create(size);
        block->push_back(std::make_shared<cmd_pop>(store, size));

        auto first_op = operands[0];
        switch (first_op.type)
        {
            case ZYDIS_OPERAND_TYPE_MEMORY:
            {
                block->push_back(std::make_shared<cmd_mem_write>(size, size));
                break;
            }
            case ZYDIS_OPERAND_TYPE_REGISTER:
            {
                codec::reg target_reg = static_cast<codec::reg>(first_op.reg.value);
                block->push_back(std::make_shared<cmd_context_store>(target_reg));

                break;
            }
        }
    }
}
