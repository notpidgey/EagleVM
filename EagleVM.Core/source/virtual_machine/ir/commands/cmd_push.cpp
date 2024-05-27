#include "eaglevm-core/virtual_machine/ir/commands/cmd_push.h"

namespace eagle::ir
{
    cmd_push::cmd_push(discrete_store_ptr reg_src, const ir_size reg_size)
        : base_command(command_type::vm_push), value(std::move(reg_src)), type(info_type::vm_temp_register), size(reg_size)
    {
    }

    cmd_push::cmd_push(reg_vm reg_src, const ir_size reg_size)
        : base_command(command_type::vm_push), value(reg_src), type(info_type::vm_register), size(reg_size)
    {
    }

    cmd_push::cmd_push(const uint64_t immediate, const ir_size stack_disp)
        : base_command(command_type::vm_push), value(immediate), type(info_type::immediate), size(stack_disp)
    {
    }

    cmd_push::cmd_push(uint64_t immediate)
        : base_command(command_type::vm_push), value(immediate), type(info_type::address), size(ir_size::bit_64)
    {
    }

    info_type cmd_push::get_push_type() const
    {
        return type;
    }

    discrete_store_ptr cmd_push::get_value_temp_register()
    {
        return std::get<discrete_store_ptr>(value);
    }

    reg_vm cmd_push::get_value_register() const
    {
        return std::get<reg_vm>(value);
    }

    uint64_t cmd_push::get_value_immediate() const
    {
        return std::get<uint64_t>(value);
    }

    ir_size cmd_push::get_size() const
    {
        return size;
    }
}
