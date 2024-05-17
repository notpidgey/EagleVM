#include "eaglevm-core/virtual_machine/ir/commands/cmd_push.h"

namespace eagle::ir
{
    cmd_push::cmd_push(discrete_store_ptr reg_src, const ir_size reg_size)
    : base_command(command_type::vm_push), reg(std::move(reg_src)), rip_relative(false), size(reg_size)
    {
        type = stack_type::vm_register;
    }

    cmd_push::cmd_push(const uint64_t immediate, const ir_size stack_disp, const bool rip_relative)
    : base_command(command_type::vm_push), value(immediate), rip_relative(rip_relative), size(stack_disp)
    {
        type = stack_type::immediate;
    }

    void cmd_push::set_modifier(ir::modifier mod)
    {
        modifier = mod;
    }

    stack_type cmd_push::get_push_type() const
    {
        return type;
    }

    ir_size cmd_push::get_value_immediate_size() const
    {
        return size;
    }

    discrete_store_ptr cmd_push::get_value_register()
    {
        return reg;
    }

    uint64_t cmd_push::get_value_immediate() const
    {
        return value;
    }
}
