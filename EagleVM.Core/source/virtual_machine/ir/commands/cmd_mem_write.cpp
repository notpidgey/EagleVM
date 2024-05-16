#include "eaglevm-core/virtual_machine/ir/commands/cmd_mem_write.h"

namespace eagle::ir
{
    cmd_mem_write::cmd_mem_write(const ir_size value_size, const ir_size write_size)
        : base_command(command_type::vm_mem_write), write_size(write_size), value_size(value_size)
    {
    }

    ir_size cmd_mem_write::get_value_size() const
    {
        return value_size;
    }

    ir_size cmd_mem_write::get_write_size() const
    {
        return write_size;
    }
}
