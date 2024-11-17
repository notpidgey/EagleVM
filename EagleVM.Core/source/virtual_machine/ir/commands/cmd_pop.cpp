#include "eaglevm-core/virtual_machine/ir/commands/cmd_pop.h"

namespace eagle::ir
{
    cmd_pop::cmd_pop(const ir_size size)
        : base_command(command_type::vm_pop), size(size)
    {
    }

    ir_size cmd_pop::get_size() const
    {
        return size;
    }

    bool cmd_pop::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_pop>(other);
        return base_command::is_similar(other) &&
            get_size() == cmd->get_size();
    }

    std::string cmd_pop::to_string()
    {
        return base_command::to_string() + "(" + ir_size_to_string(size) + ")";
    }
}
