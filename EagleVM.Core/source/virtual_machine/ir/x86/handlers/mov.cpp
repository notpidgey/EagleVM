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
    translate_mem_result mov::translate_mem_action(const codec::dec::op_mem& op_mem, uint8_t idx)
    {
        if (idx == 0) return translate_mem_result::address;
        return base_x86_translator::translate_mem_action(op_mem, idx);
    }

    void mov::finalize_translate_to_virtual()
    {
        codec::dec::operand first_op = operands[0];
        switch (first_op.type)
        {
            case ZYDIS_OPERAND_TYPE_REGISTER:
            {
                codec::reg reg = static_cast<codec::reg>(first_op.reg.value);
                if(static_cast<ir_size>(first_op.size) == ir_size::bit_32)
                    reg = codec::get_bit_version(first_op.reg.value, codec::gpr_64);

                block->add_command(std::make_shared<cmd_context_store>(reg, static_cast<codec::reg_size>(first_op.size)));

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

    bool mov::skip(const uint8_t idx)
    {
        return idx == 0 && operands[idx].type == ZYDIS_OPERAND_TYPE_REGISTER;
    }
}
