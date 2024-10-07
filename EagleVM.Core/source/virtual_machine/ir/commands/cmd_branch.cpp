#include "eaglevm-core/virtual_machine/ir/commands/cmd_branch.h"

namespace eagle::ir
{
    cmd_branch::cmd_branch(const il_exit_result& result_fallthrough)
        : base_command(command_type::vm_branch), condition(exit_condition::jmp), invert_condition(false)
    {
        info.push_back(result_fallthrough);
    }

    cmd_branch::cmd_branch(const il_exit_result& result_fallthrough, const il_exit_result& result_conditional, const exit_condition condition,
        const bool invert_condition)
        : base_command(command_type::vm_branch), condition(condition), invert_condition(invert_condition)
    {
        info.reserve(2);
        info.push_back(result_fallthrough);
        info.push_back(result_conditional);
    }

    exit_condition cmd_branch::get_condition() const
    {
        return condition;
    }

    il_exit_result& cmd_branch::get_condition_default()
    {
        // the default condition which is always a fall through to the next bb or jmp will be at the back
        return info.back();
    }

    il_exit_result& cmd_branch::get_condition_special()
    {
        return info.front();
    }

    bool cmd_branch::branch_visits(const uint64_t rva)
    {
        auto check_block = [&](const il_exit_result& exit_result) -> bool
        {
            if (std::holds_alternative<uint64_t>(exit_result))
            {
                const uint64_t exit_rva = std::get<uint64_t>(exit_result);
                return exit_rva == rva;
            }

            return false;
        };

        return check_block(get_condition_default()) | check_block(get_condition_special());
    }

    bool cmd_branch::branch_visits(const block_ptr& block)
    {
        auto check_block = [&](const il_exit_result& exit_result) -> bool
        {
            if (std::holds_alternative<block_ptr>(exit_result))
            {
                const block_ptr exit_block = std::get<block_ptr>(exit_result);
                return exit_block == block;
            }

            return false;
        };

        return check_block(get_condition_default()) | check_block(get_condition_special());
    }

    void cmd_branch::rewrite_branch(const il_exit_result& search, const il_exit_result& target)
    {
        auto check_block = [&](il_exit_result& exit_result)
        {
            if (search == exit_result)
                exit_result = target;

            return false;
        };

        check_block(get_condition_default());
        check_block(get_condition_special());
    }

    bool cmd_branch::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_branch>(other);
        return condition == cmd->get_condition() &&
            get_condition_default() == cmd->get_condition_default() &&
            get_condition_special() == cmd->get_condition_special() &&
            base_command::is_similar(other);
    }
}
