#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_stack.h"

namespace eagle::ir
{
    class cmd_context_rflags_load : public base_command
    {
    public:
        explicit cmd_context_rflags_load()
            : base_command(command_type::vm_context_rflags_load)
        {
        }

        BASE_COMMAND_CLONE(cmd_context_rflags_load);
    };

    SHARED_DEFINE(cmd_context_rflags_load);
}
