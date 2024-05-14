#include "eaglevm-core/virtual_machine/ir/commands/cmd_branch.h"

namespace eagle::ir
{
    cmd_branch::cmd_branch(const il_exit_result& result_info, const exit_condition exit_condition)
        : base_command(command_type::vm_branch)
    {
        info_size = 1;
        info[0] = result_info;
        condition = exit_condition;
    }

    cmd_branch::cmd_branch(const std::vector<il_exit_result>& result_info, const exit_condition exit_condition)
        : base_command(command_type::vm_branch)
    {
        // only 2 exits should exist when a conditional jump exists
        // otherwise we construc
        if (result_info.size() <= 1)
        {
            info_size = 1;
            info[0] = result_info[0];
            condition = exit_condition;
        }
        else
        {
            info_size = 2;
            info[0] = result_info[0];
            info[1] = result_info[1];
            condition = exit_condition;
        }
    }

    exit_condition cmd_branch::get_condition() const
    {
        return condition;
    }

    il_exit_result cmd_branch::get_condition_default()
    {
        // the default condition which is always a fall through to the next bb or jmp will be at the back
        return info.back();
    }

    il_exit_result cmd_branch::get_condition_special()
    {
        return info.front();
    }
}
