#pragma once
#include "eaglevm-core/virtual_machine/il/commands/models/cmd_modifier.h"
#include "eaglevm-core/virtual_machine/il/commands/models/cmd_stack.h"
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"
#include "eaglevm-core/virtual_machine/il/models/il_reg.h"

namespace eagle::il
{
    class cmd_vm_push : public base_command
    {
    public:
        explicit cmd_vm_push(const reg_vm reg_src, const il_size reg_size)
            : base_command(command_type::vm_push), reg(reg_src), size(reg_size), rip_relative(false)
        {
            type = stack_type::vm_register;
        }

        explicit cmd_vm_push(const uint64_t immediate, const il_size stack_disp, const bool rip_relative = false)
            : base_command(command_type::vm_push), value(immediate), rip_relative(rip_relative), size(stack_disp)
        {
            type = stack_type::immediate;
        }

        void set_modifier(modifier mod)
        {
            modifier = mod;
        }

    private:
        reg_vm reg = reg_vm::none;
        uint64_t value = 0;
        bool rip_relative;

        stack_type type;
        il_size size;

        std::optional<modifier> modifier = std::nullopt;
    };
}