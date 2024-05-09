#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::il
{
    class cmd_enter : public base_command
    {
    public:
        explicit cmd_enter()
            : base_command(command_type::vm_enter)
        {
        }

    private:
    };
}
