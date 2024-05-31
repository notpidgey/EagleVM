#include "eaglevm-core/virtual_machine/ir/x86/handlers/imul.h"

#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_store.h"

namespace eagle::ir::handler
{
    imul::imul()
    {
        valid_operands = {
            { { { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_16 } }, "imul 16,16" },
            { { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_32 } }, "imul 32,32" },
            { { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_64 } }, "imul 64,64" },

            { { { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_8 } }, "imul 16,16" },
            { { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_8 } }, "imul 32,32" },
            { { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_8 } }, "imul 64,64" },

            { { { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_16 } }, "imul 16,16" },
            { { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_32 } }, "imul 32,32" },
            { { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_32 } }, "imul 64,64" },
        };

        build_options = {
            { { ir_size::bit_16, ir_size::bit_16 }, "imul 16,16" },
            { { ir_size::bit_32, ir_size::bit_32 }, "imul 32,32" },
            { { ir_size::bit_64, ir_size::bit_64 }, "imul 64,64" },
        };
    }

    ir_insts imul::gen_handler(handler_sig signature)
    {
        assert(signature.size() == 2, "invalid signature. must contain 2 operands");
        assert(signature[0] == signature[1], "invalid signature. must contain same sized parameters");

        ir_size target_size = signature.front();

        const discrete_store_ptr vtemp = discrete_store::create(target_size);
        const discrete_store_ptr vtemp2 = discrete_store::create(target_size);

        return {
            std::make_shared<cmd_pop>(vtemp, target_size),
            std::make_shared<cmd_pop>(vtemp2, target_size)->block_write(vtemp),
            std::make_shared<cmd_x86_dynamic>(codec::m_imul, vtemp, vtemp2),
            std::make_shared<cmd_push>(vtemp, target_size)
        };
    }
}

namespace eagle::ir::lifter
{
    translate_mem_result imul::translate_mem_action(const codec::dec::op_mem& op_mem, const uint8_t idx)
    {
        return idx == 1 ? translate_mem_result::value : base_x86_translator::translate_mem_action(op_mem, idx);
    }

    void imul::finalize_translate_to_virtual()
    {
        //if (inst.operand_count_visible == 1)
        //{
        //    // use the same operand twice
        //    translate_status status = translate_status::unsupported;
        //    switch (const codec::dec::operand& operand = operands[0]; operand.type)
        //    {
        //        case ZYDIS_OPERAND_TYPE_REGISTER:
        //            status = encode_operand(operand.reg, 0);
        //            break;
        //        case ZYDIS_OPERAND_TYPE_MEMORY:
        //            status = encode_operand(operand.mem, 0);
        //            break;
        //    }

        //    assert(status == translate_status::success, "failed to virtualized operand");
        //}

        block->add_command(std::make_shared<cmd_rflags_load>());
        base_x86_translator::finalize_translate_to_virtual();
        block->add_command(std::make_shared<cmd_rflags_store>());

        codec::dec::operand first_op = operands[0];
        switch (inst.operand_count_visible)
        {
            case 2:
            case 3:
            {
                // 2:
                // the product is at the top of the stack

                // 3:
                // when there are 3 operands
                // op1 and op2 get imuld
                // then, we store in op0 which will be a reg

                // product of op1 and op2 is already on stack
                // store in op0

                block->add_command(std::make_shared<cmd_context_store>(static_cast<codec::reg>(first_op.reg.value)));

                break;
            }
            default:
            {
                assert("operand count not supported by handler");

                break;
            }
        }
    }

    bool imul::skip(const uint8_t idx)
    {
        return idx == 0 && inst.operand_count_visible == 3;
    }
}
