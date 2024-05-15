#include "eaglevm-core/virtual_machine/ir/commands/cmd_sx.h"

namespace eagle::ir
{
    cmd_sx::cmd_sx(const il_size to, const il_size from)
        : base_command(command_type::vm_sx), target(to), current(from)
    {
    }

    il_size cmd_sx::get_target() const
    {
        return target;
    }

    il_size cmd_sx::get_current() const
    {
        return current;
    }
}
