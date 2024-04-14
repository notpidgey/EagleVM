#include "eaglevm-core/virtual_machine/il/x86/base_handler_gen.h"

namespace eagle::il::handler
{
    bool base_handler_gen::get_handler(const std::vector<handler_op>& target_operands) const
    {
        const auto target_operands_len = target_operands.size();
        for (const auto& accepted_ops : entries)
        {
            if (accepted_ops.size() != target_operands.size())
                continue;

            bool is_match = true;
            for (int i = 0; i < target_operands_len; i++)
            {
                const auto target_op = target_operands[i];
                const auto accepted_op = accepted_ops[i];

                // this can be of any type if the target is none
                if (target_op.operand_type == codec::op_none)
                    continue;

                if (target_op.operand_size != accepted_op.operand_size ||
                    target_op.operand_type != accepted_op.operand_type)
                {
                    is_match = false;
                    break;
                }
            }

            if (is_match)
                return true;
        }

        return false;
    }
}
