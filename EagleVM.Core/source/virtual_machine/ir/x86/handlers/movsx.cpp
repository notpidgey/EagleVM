#include "eaglevm-core/virtual_machine/ir/x86/handlers/movsx.h"

#include "eaglevm-core/compiler/code_label.h"
#include "eaglevm-core/virtual_machine/ir/models/ir_store.h"

#include "eaglevm-core/virtual_machine/ir/block_builder.h"

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
    translate_status movsx::encode_operand(codec::dec::op_mem op_mem, uint8_t idx)
    {
        auto res = base_x86_translator::encode_operand(op_mem, idx);
        if (idx == 1)
        {
            block->push_back(std::make_shared<cmd_sx>(
                static_cast<ir_size>(operands[0].size),
                static_cast<ir_size>(operands[1].size)
            ));
        }

        return res;
    }

    translate_status movsx::encode_operand(codec::dec::op_reg op_reg, uint8_t idx)
    {
        auto res = base_x86_translator::encode_operand(op_reg, idx);
        if (idx == 1)
        {
            block->push_back(std::make_shared<cmd_sx>(
                static_cast<ir_size>(operands[0].size),
                static_cast<ir_size>(operands[1].size)
            ));
        }

        return res;
    }

    translate_mem_result movsx::translate_mem_action(const codec::dec::op_mem& op_mem, uint8_t idx)
    {
        if (idx == 0) return translate_mem_result::address;
        return base_x86_translator::translate_mem_action(op_mem, idx);
    }

    void movsx::finalize_translate_to_virtual(x86_cpu_flag flags)
    {
        codec::dec::operand first_op = operands[0];

        // always will be a reg
        codec::reg reg = static_cast<codec::reg>(first_op.reg.value);
        if (static_cast<ir_size>(first_op.size) == ir_size::bit_32)
        {
            reg = codec::get_bit_version(first_op.reg.value, codec::gpr_64);
            block->push_back(std::make_shared<cmd_resize>(ir_size::bit_64, ir_size::bit_32));
            block->push_back(std::make_shared<cmd_context_store>(reg));
        }
        else
        {
            block->push_back(std::make_shared<cmd_context_store>(reg));
        }

        // no handler call required
        // base_x86_translator::finalize_translate_to_virtual();
    }

    bool movsx::skip(const uint8_t idx)
    {
        return idx == 0 && operands[idx].type == ZYDIS_OPERAND_TYPE_REGISTER;
    }
}
