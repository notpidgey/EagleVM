#include "eaglevm-core/virtual_machine/ir/commands/cmd_pop.h"

namespace eagle::ir
{
    cmd_pop::cmd_pop(discrete_store_ptr reg_dest, ir_size size)
        : base_command(command_type::vm_pop), dest_reg(std::move(reg_dest)), size(size)
    {
    }

    discrete_store_ptr cmd_pop::get_destination_reg()
    {
        return dest_reg;
    }

    ir_size cmd_pop::get_size() const
    {
        return size;
    }
}
