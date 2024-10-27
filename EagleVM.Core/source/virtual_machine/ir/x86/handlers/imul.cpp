#include "eaglevm-core/virtual_machine/ir/x86/handlers/imul.h"

#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_rflags_store.h"
#include "eaglevm-core/virtual_machine/ir/x86/handlers/util/flags.h"

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
        VM_ASSERT(signature.size() == 2, "invalid signature. must contain 2 operands");
        VM_ASSERT(signature[0] == signature[1], "invalid signature. must contain same sized parameters");

        ir_size target_size = signature.front();

        constexpr auto affected_flags = ZYDIS_CPUFLAG_CF | ZYDIS_CPUFLAG_OF;
        ir_insts insts = {
            std::make_shared<cmd_smul>(target_size, false, true),

            /*
                For the one operand form of the instruction, the CF and OF flags are set when significant bits are carried into the upper half of the
                result and cleared when the result fits exactly in the lower half of the result. For the two- and three-operand forms of the instruction,
                the CF and OF flags are set when the result must be truncated to fit in the destination operand size and cleared when the result fits
                exactly in the destination operand size. The SF, ZF, AF, and PF flags are undefined.
            */
            std::make_shared<cmd_context_rflags_load>(),
            std::make_shared<cmd_push>(~affected_flags, ir_size::bit_64),
            std::make_shared<cmd_and>(ir_size::bit_64),
        };

        insts.append_range(compute_of_cf(target_size));
        return insts;
    }

    ir_insts imul::compute_of_cf(ir_size size)
    {
        ir_insts insts;
        insts.append_range(copy_to_top(size, util::param_one));
        insts.append_range(copy_to_top(size, util::param_two));

        return {
            std::make_shared<cmd_
        };
    }
}

namespace eagle::ir::lifter
{
    translate_mem_result imul::translate_mem_action(const codec::dec::op_mem& op_mem, const uint8_t idx)
    {
        return idx == 1 ? translate_mem_result::value : base_x86_translator::translate_mem_action(op_mem, idx);
    }

    void imul::finalize_translate_to_virtual(const x86_cpu_flag flags)
    {
        base_x86_translator::finalize_translate_to_virtual(flags);

        codec::dec::operand first_op = operands[0];
        switch (inst.operand_count_visible)
        {
            case 2:
            case 3:
            {
                block->push_back(std::make_shared<cmd_context_store>(static_cast<codec::reg>(first_op.reg.value)));
                break;
            }
            default:
            {
                VM_ASSERT("operand count not supported by handler");

                break;
            }
        }

        // clean up regs on stack due to handler leaving params
        const ir_size target_size = static_cast<ir_size>(first_op.size);
        block->push_back(std::make_shared<cmd_pop>(target_size));
        block->push_back(std::make_shared<cmd_pop>(target_size));
    }

    bool imul::skip(const uint8_t idx) { return idx == 0 && inst.operand_count_visible == 3; }
}
