#include "eaglevm-core/virtual_machine/ir/commands/cmd_branch.h"

namespace eagle::ir
{
    cmd_branch::cmd_branch(const il_exit_result& result_info)
        : base_command(command_type::vm_branch), condition(exit_condition::jump)
    {
        info.push_back(result_info);
        condition = exit_condition::jump;
    }

    cmd_branch::cmd_branch(const std::vector<il_exit_result>& result_info)
        : base_command(command_type::vm_branch)
    {
        assert(result_info.size() == 2, "cannot have a conditional exit with more/less than 2 exits");

        info.push_back(result_info[0]);
        info.push_back(result_info[1]);
        condition = exit_condition::conditional;
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
