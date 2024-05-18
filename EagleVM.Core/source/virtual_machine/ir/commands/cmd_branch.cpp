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
        assert(result_info.size() <= 2, "cannot have more than 2 exiting branches");
        for(auto& exit : result_info)
            info.push_back(exit);
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
