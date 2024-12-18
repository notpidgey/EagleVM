#include "eaglevm-core/virtual_machine/ir/commands/cmd_carry.h"

namespace eagle::ir
{
    cmd_carry::cmd_carry(ir_size value_size, uint16_t byte_move_size)
    : base_command(command_type::vm_carry), size(value_size), byte_move_size(byte_move_size)
    {
    }

    ir_size cmd_carry::get_size() const
    {
        return size;
    }

    uint16_t cmd_carry::get_move_size() const
    {
        return byte_move_size;
    }

    bool cmd_carry::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_carry>(other);
        return base_command::is_similar(other) &&
            get_size() == cmd->get_size();
    }

    std::string cmd_carry::to_string()
    {
        return base_command::to_string() + "(" + ir_size_to_string(size) + ")";
    }
}
