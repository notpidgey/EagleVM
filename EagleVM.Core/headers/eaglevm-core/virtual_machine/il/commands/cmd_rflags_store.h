#pragma once
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"
#include "eaglevm-core/virtual_machine/il/commands/models/cmd_stack.h"

namespace eagle::il
{
    class cmd_rflags_store : public base_command
    {
    public:
        explicit cmd_rflags_store()
            : base_command(command_type::vm_rflags_store)
        {
        }
    };
}
