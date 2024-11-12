#include "eaglevm-core/virtual_machine/ir/commands/cmd_mem_read.h"
#include <format>

namespace eagle::ir
{
    cmd_mem_read::cmd_mem_read(const ir_size size)
        : base_command(command_type::vm_mem_read), size(size)
    {
    }

    ir_size cmd_mem_read::get_read_size() const
    {
        return size;
    }

    bool cmd_mem_read::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_mem_read>(other);
        return base_command::is_similar(other) &&
            get_read_size() == cmd->get_read_size();
    }

    std::string cmd_mem_read::to_string()
    {
        return base_command::to_string() + std::format(" [{}]", ir_size_to_string(size));
    }
}
