#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_vm_enter : public base_command
    {
    public:
        explicit cmd_vm_enter()
            : base_command(command_type::vm_enter)
        {
        }

        BASE_COMMAND_CLONE(cmd_vm_enter);

    private:
    };

    SHARED_DEFINE(cmd_vm_enter);
}
