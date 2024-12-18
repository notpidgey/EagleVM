#include "eaglevm-core/virtual_machine/ir/x86/handlers/inc.h"
#include "eaglevm-core/virtual_machine/ir/x86/handlers/util/flags.h"

#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_rflags_store.h"

#include "eaglevm-core/virtual_machine/ir/block_builder.h"

namespace eagle::ir::handler
{
    inc::inc()
    {
        valid_operands = {
            { { { codec::op_none, codec::bit_8 } }, "inc 8" },
            { { { codec::op_none, codec::bit_16 } }, "inc 16" },
            { { { codec::op_none, codec::bit_32 } }, "inc 32" },
            { { { codec::op_none, codec::bit_64 } }, "inc 64" },
        };

        build_options = {
            { { ir_size::bit_8 }, "inc 8" },
            { { ir_size::bit_16 }, "inc 16" },
            { { ir_size::bit_32 }, "inc 32" },
            { { ir_size::bit_64 }, "inc 64" },
        };
    }

    ir_insts inc::gen_handler(const handler_sig signature)
    {
        VM_ASSERT(signature.size() == 1, "invalid signature. must contain 1 operand");
        const ir_size target_size = signature.front();

        constexpr auto affected_flags = ZYDIS_CPUFLAG_OF | ZYDIS_CPUFLAG_SF | ZYDIS_CPUFLAG_ZF | ZYDIS_CPUFLAG_AF | ZYDIS_CPUFLAG_PF;
        block_builder builder;
        builder
            .add_push(1, target_size)
            .add_add(target_size, false, true)

            // The CF flag is not affected. The OF, SF, ZF, AF, and PF flags are set according to the result.
            .add_context_rflags_load()
            .add_push(~affected_flags, ir_size::bit_64)
            .add_and(ir_size::bit_64)

            .append(util::calculate_sf(target_size))
            .append(util::calculate_zf(target_size))
            .append(util::calculate_pf(target_size))

            .append(compute_of(target_size))
            .append(compute_af(target_size));

        return builder.build();
    }

    ir_insts inc::compute_of(ir_size size)
    {
        block_builder builder;
        builder
            .append(copy_to_top(size, util::param_two))
            .add_push(static_cast<uint64_t>(size) - 1, size)
            .add_shr(size)

            .append(copy_to_top(size, util::result, { size }))
            .add_push(static_cast<uint64_t>(size) - 1, size)
            .add_shr(size)

            .add_cmp(size)
            .add_flags_load(vm_flags::eq)
            .add_push(1, ir_size::bit_64)
            .add_xor(ir_size::bit_64)
            .add_push(util::flag_index(ZYDIS_CPUFLAG_OF), ir_size::bit_64)
            .add_shl(ir_size::bit_64)
            .add_or(ir_size::bit_64);

        return builder.build();
    }

    ir_insts inc::compute_af(ir_size size)
    {
        block_builder builder;
        builder
            .append(copy_to_top(size, util::param_two))
            .add_push(0xF, size)
            .add_and(size)
            .add_push(1, size)
            .add_add(size)
            .add_push(0x10, size)
            .add_and(size)

            .add_push(0, size)
            .add_cmp(size)

            .add_flags_load(vm_flags::eq)
            .add_push(1, ir_size::bit_64)
            .add_xor(ir_size::bit_64)
            .add_push(util::flag_index(ZYDIS_CPUFLAG_AF), ir_size::bit_64)
            .add_shl(ir_size::bit_64)
            .add_or(ir_size::bit_64);

        return builder.build();
    }
}

namespace eagle::ir::lifter
{
    bool inc::translate_to_il(uint64_t original_rva, x86_cpu_flag flags)
    {
        return base_x86_translator::translate_to_il(original_rva, flags);
    }

    translate_mem_result inc::translate_mem_action(const codec::dec::op_mem& op_mem, uint8_t idx)
    {
        return translate_mem_result::both;
    }

    void inc::finalize_translate_to_virtual(const x86_cpu_flag flags)
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

            if (reg == codec::rsp)
                return;

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
