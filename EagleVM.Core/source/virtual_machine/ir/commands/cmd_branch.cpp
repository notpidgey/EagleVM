#include "eaglevm-core/virtual_machine/ir/commands/cmd_branch.h"

namespace eagle::ir
{
    cmd_branch::cmd_branch()
        : base_command(command_type::vm_branch)
    {
    }

    bool cmd_branch::is_similar(const std::shared_ptr<base_command>& other)
    {
        return true;
    }
}
