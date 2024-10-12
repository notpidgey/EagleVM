#include "eaglevm-core/virtual_machine/ir/commands/cmd_push.h"

namespace eagle::ir
{
    cmd_push::cmd_push(const push_v& reg_src, const ir_size reg_size)
        : base_command(command_type::vm_push), size(reg_size), value(reg_src)
    {
    }

    cmd_push::cmd_push(uint64_t immediate, const ir_size stack_disp)
        : base_command(command_type::vm_push), size(stack_disp), value(immediate)
    {
    }

    ir_size cmd_push::get_size() const
    {
        return size;
    }

    push_v cmd_push::get_value() const
    {
        return value;
    }

    bool cmd_push::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_push>(other);
        return base_command::is_similar(other) &&
            get_size() == cmd->get_size() &&
            value == cmd->value;
    }

    std::vector<discrete_store_ptr> cmd_push::get_use_stores()
    {
        if (std::holds_alternative<discrete_store_ptr>(value))
            return { std::get<discrete_store_ptr>(value) };

        return { };
    }
}
