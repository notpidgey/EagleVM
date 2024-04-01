#include "eaglevm-core/virtual_machine/il/commands/cmd_exit.h"

namespace eagle::il
{
    cmd_exit::cmd_exit(const exit_result& result_info, const exit_condition exit_condition)
        : base_command(command_type::vm_exit)
    {
        info_size = 1;
        info[0] = result_info;
        condition = exit_condition;
    }

    cmd_exit::cmd_exit(const std::vector<exit_result>& result_info, const exit_condition exit_condition)
        : base_command(command_type::vm_exit)
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

    exit_condition cmd_exit::get_condition() const
    {
        return condition;
    }

    exit_result cmd_exit::get_condition_default()
    {
        // the default condition which is always a fall through to the next bb or jmp will be at the back
        return info.back();
    }

    exit_result cmd_exit::get_condition_special()
    {
        return info.front();
    }
}
