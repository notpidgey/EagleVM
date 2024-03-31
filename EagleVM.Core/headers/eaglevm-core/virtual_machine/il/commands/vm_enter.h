#pragma once
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"

namespace eagle::il
{
    class vm_enter : public base_command
    {
    public:
        explicit vm_enter()
            : base_command(command_type::vm_enter)
        {

        }

    private:
    };
}
