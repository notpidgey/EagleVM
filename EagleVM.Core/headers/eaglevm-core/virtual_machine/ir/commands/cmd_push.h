#pragma once
#include <optional>
#include <utility>

#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_modifier.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_stack.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/models/ir_store.h"

namespace eagle::ir
{
    class cmd_push : public base_command
    {
    public:
        explicit cmd_push(discrete_store_ptr reg_src, ir_size reg_size);
        explicit cmd_push(reg_vm reg_src, ir_size reg_size);
        explicit cmd_push(uint64_t immediate, ir_size stack_disp);
        explicit cmd_push(uint64_t immediate);

        info_type get_push_type() const;
        ir_size get_size() const;

        discrete_store_ptr get_value_temp_register();
        reg_vm get_value_register() const;
        uint64_t get_value_immediate() const;

        bool is_similar(const std::shared_ptr<base_command>& other) override;

    private:
        info_type type;
        ir_size size;

        std::variant<discrete_store_ptr, reg_vm, uint64_t> value;
    };
}
