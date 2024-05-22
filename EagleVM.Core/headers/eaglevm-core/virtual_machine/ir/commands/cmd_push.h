#pragma once
#include <optional>
#include <utility>

#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_modifier.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_stack.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_push : public base_command
    {
    public:
        explicit cmd_push(discrete_store_ptr reg_src, ir_size reg_size);
        explicit cmd_push(uint64_t immediate, ir_size stack_disp, bool rip_relative = false);

        void set_modifier(modifier mod);

        stack_type get_push_type() const;
        ir_size get_value_immediate_size() const;

        discrete_store_ptr get_value_register();
        uint64_t get_value_immediate() const;

    private:
        discrete_store_ptr reg;
        uint64_t value = 0;
        bool rip_relative;

        stack_type type;
        ir_size size;

        std::optional<modifier> modifier = std::nullopt;
    };
}
