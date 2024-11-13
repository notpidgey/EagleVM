#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_stack.h"

namespace eagle::ir
{
    class cmd_sx : public base_command
    {
    public:
        explicit cmd_sx(ir_size to, ir_size from);

        ir_size get_target() const;
        ir_size get_current() const;

        bool is_similar(const std::shared_ptr<base_command>& other) override;
        BASE_COMMAND_CLONE(cmd_sx);

    private:
        ir_size target;
        ir_size current;
    };

    SHARED_DEFINE(cmd_sx);
}
