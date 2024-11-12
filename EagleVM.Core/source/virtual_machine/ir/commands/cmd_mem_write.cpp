#include "eaglevm-core/virtual_machine/ir/commands/cmd_mem_write.h"

namespace eagle::ir
{
    cmd_mem_write::cmd_mem_write(const ir_size value_size, const ir_size write_size, const bool value_nearest)
        : base_command(command_type::vm_mem_write), write_size(write_size), value_size(value_size), value_nearest(value_nearest)
    {
    }

    bool cmd_mem_write::get_is_value_nearest() const
    {
        return value_nearest;
    }

    ir_size cmd_mem_write::get_value_size() const
    {
        return value_size;
    }

    ir_size cmd_mem_write::get_write_size() const
    {
        return write_size;
    }

    bool cmd_mem_write::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_mem_write>(other);
        return base_command::is_similar(other) &&
            get_is_value_nearest() == cmd->get_is_value_nearest() &&
            get_value_size() == cmd->get_value_size() &&
            get_write_size() == cmd->get_write_size();
    }
}
