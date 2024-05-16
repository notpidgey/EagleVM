#pragma once
#include <optional>
#include <utility>

#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_modifier.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_stack.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/models/ir_reg.h"

namespace eagle::ir
{
    class cmd_push : public base_command
    {
    public:
        explicit cmd_push(discrete_store_ptr  reg_src, const ir_size reg_size)
            : base_command(command_type::vm_push), reg(std::move(reg_src)), rip_relative(false), size(reg_size)
        {
            type = stack_type::vm_register;
        }

        explicit cmd_push(const uint64_t immediate, const ir_size stack_disp, const bool rip_relative = false)
            : base_command(command_type::vm_push), value(immediate), rip_relative(rip_relative), size(stack_disp)
        {
            type = stack_type::immediate;
        }

        void set_modifier(modifier mod)
        {
            modifier = mod;
        }

    private:
        discrete_store_ptr reg;
        uint64_t value = 0;
        bool rip_relative;

        stack_type type;
        ir_size size;

        std::optional<modifier> modifier = std::nullopt;
    };
}
