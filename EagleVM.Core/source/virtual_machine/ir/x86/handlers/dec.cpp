#include "eaglevm-core/virtual_machine/ir/x86/handlers/dec.h"
#include "eaglevm-core/virtual_machine/ir/x86/handlers/util/flags.h"

#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_rflags_store.h"

#include "eaglevm-core/virtual_machine/ir/block_builder.h"

namespace eagle::ir::handler
{
    dec::dec()
    {
        valid_operands = {
            { { { codec::op_none, codec::bit_8 } }, "dec 8" },
            { { { codec::op_none, codec::bit_16 } }, "dec 16" },
            { { { codec::op_none, codec::bit_32 } }, "dec 32" },
            { { { codec::op_none, codec::bit_64 } }, "dec 64" },
        };

        build_options = {
            { { ir_size::bit_8 }, "dec 8" },
            { { ir_size::bit_16 }, "dec 16" },
            { { ir_size::bit_32 }, "dec 32" },
            { { ir_size::bit_64 }, "dec 64" },
        };
    }

    ir_insts dec::gen_handler(handler_sig signature)
    {
        VM_ASSERT(signature.size() == 1, "invalid signature. must contain 1 operand");
        ir_size target_size = signature.front();

        constexpr auto affected_flags = ZYDIS_CPUFLAG_OF | ZYDIS_CPUFLAG_SF | ZYDIS_CPUFLAG_ZF | ZYDIS_CPUFLAG_AF | ZYDIS_CPUFLAG_PF;
        ir_insts insts = {
            std::make_shared<cmd_push>(1, target_size),
            std::make_shared<cmd_sub>(target_size, false, true),

            // The CF flag is not affected. The OF, SF, ZF, AF, and PF flags are set according to the result.
            std::make_shared<cmd_context_rflags_load>(),
            std::make_shared<cmd_push>(~affected_flags, ir_size::bit_64),
            std::make_shared<cmd_and>(ir_size::bit_64),
        };

        insts.append_range(util::calculate_sf(target_size));
        insts.append_range(util::calculate_zf(target_size));
        insts.append_range(util::calculate_pf(target_size));

        insts.append_range(compute_of(target_size));
        insts.append_range(compute_af(target_size));

        return insts;
    }

    ir_insts dec::compute_of(ir_size size)
    {
        ir_insts insts;
        insts.append_range(copy_to_top(size, util::param_two));
        insts.append_range(ir_insts{
            std::make_shared<cmd_push>(static_cast<uint64_t>(size) - 1, size),
            std::make_shared<cmd_shr>(size),
        });

        insts.append_range(copy_to_top(size, util::result, { size }));
        insts.append_range(ir_insts{
            std::make_shared<cmd_push>(static_cast<uint64_t>(size) - 1, size),
            std::make_shared<cmd_shr>(size),
        });

        insts.append_range(ir_insts{
            std::make_shared<cmd_cmp>(size),

            std::make_shared<cmd_flags_load>(vm_flags::eq),
            std::make_shared<cmd_push>(1, ir_size::bit_64),
            std::make_shared<cmd_xor>(ir_size::bit_64),

            std::make_shared<cmd_push>(util::flag_index(ZYDIS_CPUFLAG_OF), ir_size::bit_64),
            std::make_shared<cmd_shl>(ir_size::bit_64),

            std::make_shared<cmd_or>(ir_size::bit_64),
        });

        return insts;
    }

    ir_insts dec::compute_af(ir_size size)
    {
        ir_insts insts;
        insts.append_range(copy_to_top(size, util::param_two));

        insts.append_range(ir_insts{
            std::make_shared<cmd_push>(0xF, size),
            std::make_shared<cmd_and>(size),

            std::make_shared<cmd_push>(0, size),
            std::make_shared<cmd_cmp>(size),

            std::make_shared<cmd_flags_load>(vm_flags::eq),
            std::make_shared<cmd_push>(util::flag_index(ZYDIS_CPUFLAG_AF), ir_size::bit_64),
            std::make_shared<cmd_shl>(ir_size::bit_64),

            std::make_shared<cmd_or>(ir_size::bit_64),
        });

        return insts;
    }
}

namespace eagle::ir::lifter
{
    translate_mem_result dec::translate_mem_action(const codec::dec::op_mem& op_mem, uint8_t idx) { return translate_mem_result::both; }

    void dec::finalize_translate_to_virtual(const x86_cpu_flag flags)
    {
        base_x86_translator::finalize_translate_to_virtual(flags);

        codec::dec::operand first_op = operands[0];
        if (first_op.type == ZYDIS_OPERAND_TYPE_REGISTER)
        {
            // register
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

            // clean up regs on stack due to handler leaving params
            const ir_size target_size = static_cast<ir_size>(first_op.size);
            block->push_back(std::make_shared<cmd_pop>(target_size));
            block->push_back(std::make_shared<cmd_pop>(target_size));
        }
        else if (first_op.type == ZYDIS_OPERAND_TYPE_MEMORY)
        {
            // carry down result

            ir_size value_size = static_cast<ir_size>(first_op.size);
            block->push_back(std::make_shared<cmd_carry>(value_size, operands[1].size / 8 * 2));
            block->push_back(std::make_shared<cmd_mem_write>(value_size, value_size, true));
        }
    }
}
