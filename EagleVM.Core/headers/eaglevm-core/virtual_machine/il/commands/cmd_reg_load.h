#pragma once
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"

namespace eagle::il
{
    class cmd_reg_load : public base_command
    {
    public:
        explicit cmd_reg_load()
            : base_command(command_type::vm_handler_call)
        {
        }

    private:
        call_type call_type = call_type::none;
    };
}
