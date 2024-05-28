#include "eaglevm-core/virtual_machine/ir/x86/handlers/mov.h"

namespace eagle::ir::handler
{
    mov::mov()
    {
        valid_operands = {
            { { { codec::op_none, codec::bit_8 }, { codec::op_none, codec::bit_8 } }, "mov 8,8" },
            { { { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_16 } }, "mov 16,16" },
            { { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_32 } }, "mov 32,32" },
            { { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_64 } }, "mov 64,64" },
        };

        // we actually dont call the vm handler here so we dont need any build options : )
        // build_options = {
        //     { { ir_size::bit_8, ir_size::bit_8 }, "mov 8,8" },
        //     { { ir_size::bit_16, ir_size::bit_16 }, "mov 16,16" },
        //     { { ir_size::bit_32, ir_size::bit_32 }, "mov 32,32" },
        //     { { ir_size::bit_64, ir_size::bit_64 }, "mov 64,64" },
        // };
    }
}

namespace eagle::ir::lifter
{
    bool mov::virtualize_as_address(const codec::dec::operand operand, const uint8_t idx)
    {
        if (idx == 0 && operand.type == codec::op_mem)
            return true;

        return false;
    }

    void mov::finalize_translate_to_virtual()
    {
        codec::dec::operand first_op = operands[0];
        switch (first_op.type)
        {
            case ZYDIS_OPERAND_TYPE_REGISTER:
            {
                codec::reg target_reg = static_cast<codec::reg>(first_op.reg.value);
                block->add_command(std::make_shared<cmd_context_store>(target_reg));

                break;
            }
            case ZYDIS_OPERAND_TYPE_MEMORY:
            {
                ir_size target_size = static_cast<ir_size>(first_op.size);
                block->add_command(std::make_shared<cmd_mem_write>(target_size, target_size));

                break;
            }
        }

        // no handler call required
        // base_x86_translator::finalize_translate_to_virtual();
    }
}