#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"

namespace eagle::ir::handler
{
    ir_insts base_handler_gen::gen_handler(const uint64_t target_handler_id)
    {
        std::optional<handler_sig> something = std::nullopt;
        for (const auto& [size, handler_id] : build_options)
        {
            if (handler_id == target_handler_id)
                something = size;
        }

        if (something == std::nullopt)
        {
            VM_ASSERT("invalid target handler id");
            return { };
        }

        return gen_handler(something.value());
    }

    ir_insts base_handler_gen::gen_handler(handler_sig)
    {
        VM_ASSERT("unimplemented gen_handler. unable to generate handler for signature");
        return { };
    }

    std::optional<uint64_t> base_handler_gen::get_handler_id(const op_params& target_operands)
    {
        const auto target_operands_len = target_operands.size();
        for (const auto& [entries, handler_id] : valid_operands)
        {
            op_params accepted_ops = entries;
            if (accepted_ops.size() != target_operands.size())
                continue;

            bool is_match = true;
            for (int i = 0; i < target_operands_len; i++)
            {
                const auto target_op = target_operands[i];
                const auto accepted_op = accepted_ops[i];

                // this can be of any type if the target is none
                const bool type_match = accepted_op.operand_type == codec::op_none || target_op.operand_type == accepted_op.operand_type;
                const bool size_match = target_op.operand_size == accepted_op.operand_size;

                if (!type_match || !size_match)
                {
                    is_match = false;
                    break;
                }
            }

            if (is_match)
                return handler_id;
        }

        return std::nullopt;
    }

    std::optional<uint64_t> base_handler_gen::get_handler_id(const handler_params& target_build)
    {
        for (auto [params, handler_id] : build_options)
            if (params == target_build)
                return handler_id;

        return std::nullopt;
    }

    std::optional<handler_build> base_handler_gen::get_handler_build(uint64_t target_handler_id) const
    {
        for (handler_build entry : build_options)
            if (entry.handler_id == target_handler_id)
                return entry;

        return std::nullopt;
    }
}
