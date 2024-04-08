#pragma once
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"
#include "eaglevm-core/virtual_machine/il/commands/models/cmd_stack.h"

namespace eagle::il
{
    class cmd_sx : public base_command
    {
    public:
        explicit cmd_sx(const il_size to, const il_size from)
            : base_command(command_type::vm_sx), target(to), current(from)
        {
        }

    private:
        il_size target;
        il_size current;
    };
}
