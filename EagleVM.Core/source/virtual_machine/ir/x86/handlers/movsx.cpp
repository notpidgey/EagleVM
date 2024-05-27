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
        if (op_mem.base == ZYDIS_REGISTER_RIP)
        {
            auto [target, _] = codec::calc_relative_rva(inst, operands, orig_rva, idx);
            block->add_command(std::make_shared<cmd_push>(target));

            return translate_status::success;
        }

        // [base + index * scale + disp]
        // 1. loading the base register
        if (op_mem.base == ZYDIS_REGISTER_RSP)
        {
            block->add_command(std::make_shared<cmd_push>(reg_vm::vsp, ir_size::bit_64));
            if (stack_displacement)
            {
                block->add_command(std::make_shared<cmd_push>(stack_displacement, ir_size::bit_64));
                block->add_command(std::make_shared<cmd_handler_call>(codec::m_add, handler_sig{ ir_size::bit_64, ir_size::bit_64 }));
            }
        }
        else
        {
            block->add_command(std::make_shared<cmd_context_load>(static_cast<codec::reg>(op_mem.base)));
        }

        //2. load the index register and multiply by scale
        //mov VTEMP, imm    ;
        //jmp VM_LOAD_REG   ; load value of INDEX reg to the top of the VSTACK
        if (op_mem.index != ZYDIS_REGISTER_NONE)
        {
            block->add_command(std::make_shared<cmd_context_load>(static_cast<codec::reg>(op_mem.index)));
        }

        if (op_mem.scale != 0)
        {
            block->add_command(std::make_shared<cmd_push>(op_mem.scale, ir_size::bit_64));
            block->add_command(std::make_shared<cmd_handler_call>(codec::m_imul, handler_sig{ ir_size::bit_64, ir_size::bit_64 }));
        }

        if (op_mem.index != ZYDIS_REGISTER_NONE)
        {
            block->add_command(std::make_shared<cmd_handler_call>(codec::m_add, handler_sig{ ir_size::bit_64, ir_size::bit_64 }));
        }

        if (op_mem.disp.has_displacement)
        {
            // 3. load the displacement and add
            // we can do this with some trickery using LEA so we dont modify rflags

            // subtract displacement value
            block->add_command(std::make_shared<cmd_push>(op_mem.disp.value, ir_size::bit_64));
            block->add_command(std::make_shared<cmd_handler_call>(codec::m_sub, handler_sig{ ir_size::bit_64, ir_size::bit_64 }));
        }

        // for memory operands we will only ever need one kind of action
        // there has to be a better and cleaner way of doing this, but i have not thought of it yet
        // for now it will kind of just be an assumption

        if (op_mem.type == ZYDIS_MEMOP_TYPE_AGEN || virtualize_as_address(operands[idx], idx))
        {
            stack_displacement += static_cast<uint16_t>(ir_size::bit_64);
        }
        else
        {
            // by default, this will be dereferenced and we will get the value at the address,
            const ir_size target_size = static_cast<ir_size>(inst.operand_width);
            block->add_command(std::make_shared<cmd_mem_read>(target_size));

            stack_displacement += static_cast<uint16_t>(target_size);
        }

        const ir_size target = get_op_width();
        const ir_size size = static_cast<ir_size>(operands[idx].size);

        block->add_command(std::make_shared<cmd_sx>(target, size));

        return translate_status::success;
    }
}
