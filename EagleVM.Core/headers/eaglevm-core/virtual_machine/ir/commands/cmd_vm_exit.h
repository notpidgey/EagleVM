#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_vm_exit : public base_command
    {
    public:
        explicit cmd_vm_exit()
            : base_command(command_type::vm_exit)
        {
        }

    private:
    };

    SHARED_DEFINE(cmd_vm_exit);
}
