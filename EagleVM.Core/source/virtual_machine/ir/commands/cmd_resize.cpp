#include "eaglevm-core/virtual_machine/ir/commands/cmd_resize.h"

namespace eagle::ir
{
    cmd_resize::cmd_resize(const ir_size to, const ir_size from)
        : base_command(command_type::vm_resize), target(to), current(from)
    {
    }

    ir_size cmd_resize::get_target() const
    {
        return target;
    }

    ir_size cmd_resize::get_current() const
    {
        return current;
    }

    bool cmd_resize::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_resize>(other);
        return base_command::is_similar(other) &&
            get_target() == cmd->get_target() &&
            get_current() == cmd->get_current();
    }
}
