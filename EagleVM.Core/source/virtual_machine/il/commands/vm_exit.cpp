#include "eaglevm-core/virtual_machine/il/commands/vm_exit.h"

namespace eagle::il
{
    vm_exit::vm_exit(const exit_result_info result_info, const exit_condition exit_condition)
        : base_command(command_type::vm_exit)
    {
        info.push_back(result_info);
        condition = exit_condition;
    }

    vm_exit::vm_exit(const std::vector<exit_result_info>& result_info, const exit_condition exit_condition)
        : base_command(command_type::vm_exit)
    {
        // only 2 exits should exist when a conditional jump exists
        // otherwise we construc
        if (result_info.size() == 1)
        {
            info = result_info;
            condition = exit_condition::exit;
        }
        else
        {
            info = result_info;
            condition = exit_condition;
        }
    }

    exit_condition vm_exit::get_condition()
    {
        return condition;
    }

    exit_result_v vm_exit::get_condition_default()
    {
    }

    exit_result_v vm_exit::get_condition_special()
    {
    }
}
