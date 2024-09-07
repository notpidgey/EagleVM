#include "eaglevm-core/virtual_machine/ir/commands/cmd_branch.h"

namespace eagle::ir
{
    cmd_branch::cmd_branch(const il_exit_result& result_info, const exit_condition condition)
        : base_command(command_type::vm_branch), condition(condition)
    {
        info.push_back(result_info);
    }

    cmd_branch::cmd_branch(const std::vector<il_exit_result>& result_info, const exit_condition condition)
        : base_command(command_type::vm_branch), condition(condition)
    {
        VM_ASSERT(result_info.size() <= 2, "cannot have more than 2 exiting branches");
        for (auto& exit : result_info)
            info.push_back(exit);
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

    bool cmd_branch::branch_visits(const vmexit_rva rva)
    {
        auto check_block = [&](const il_exit_result& exit_result) -> bool
        {
            if (std::holds_alternative<vmexit_rva>(exit_result))
            {
                const vmexit_rva exit_rva = std::get<vmexit_rva>(exit_result);
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
