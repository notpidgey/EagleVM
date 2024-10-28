#include "eaglevm-core/virtual_machine/ir/commands/cmd_pop.h"

namespace eagle::ir
{
    cmd_pop::cmd_pop(discrete_store_ptr reg_dest, const ir_size size)
        : base_command(command_type::vm_pop), dest_reg(std::move(reg_dest)), size(size)
    {
    }

    cmd_pop::cmd_pop(const ir_size size)
        : base_command(command_type::vm_pop), dest_reg(nullptr), size(size)
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

    bool cmd_pop::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_pop>(other);
        return base_command::is_similar(other) &&
            get_destination_reg() == cmd->get_destination_reg() &&
            get_size() == cmd->get_size();
    }

    std::vector<discrete_store_ptr> cmd_pop::get_use_stores()
    {
        if (dest_reg != nullptr)
            return { dest_reg };
        return { };
    }
}
