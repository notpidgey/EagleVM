#pragma once
#include <optional>
#include <utility>

#include "eaglevm-core/virtual_machine/ir/block.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_modifier.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/models/ir_store.h"

namespace eagle::ir
{
    using push_v = std::variant<uint64_t, block_ptr, discrete_store_ptr, reg_vm>;
    class cmd_push : public base_command
    {
    public:
        explicit cmd_push(const push_v& reg_src, ir_size reg_size);
        explicit cmd_push(uint64_t immediate, ir_size stack_disp);

        ir_size get_size() const;
        push_v get_value() const;

        bool is_similar(const std::shared_ptr<base_command>& other) override;

        std::vector<discrete_store_ptr> get_use_stores() override;

    private:
        ir_size size;
        push_v value;
    };

    SHARED_DEFINE(cmd_push);
}
