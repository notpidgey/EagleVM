#include "eaglevm-core/virtual_machine/ir/commands/cmd_sx.h"

namespace eagle::ir
{
    cmd_sx::cmd_sx(const ir_size to, const ir_size from)
        : base_command(command_type::vm_sx), target(to), current(from)
    {
        VM_ASSERT(to >= from, "sign extension must grow");
    }

    ir_size cmd_sx::get_target() const
    {
        return target;
    }

    ir_size cmd_sx::get_current() const
    {
        return current;
    }

    bool cmd_sx::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_sx>(other);
        return base_command::is_similar(other) &&
            get_target() == cmd->get_target() &&
            get_current() == cmd->get_current();
    }
}
