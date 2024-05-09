#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_stack.h"

namespace eagle::il
{
    class cmd_rflags_load : public base_command
    {
    public:
        explicit cmd_rflags_load()
            : base_command(command_type::vm_rflags_load)
        {
        }
    };
}
