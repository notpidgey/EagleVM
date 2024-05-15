#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_stack.h"

namespace eagle::ir
{
    class cmd_sx : public base_command
    {
    public:
        explicit cmd_sx(il_size to, il_size from);

        il_size get_target() const;
        il_size get_current() const;

    private:
        il_size target;
        il_size current;
    };
}
