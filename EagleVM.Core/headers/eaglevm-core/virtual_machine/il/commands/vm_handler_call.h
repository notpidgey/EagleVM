#pragma once
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"

namespace eagle::il
{
    enum class call_type
    {
        none,
        inst_handler,
        vm_handler
    };

    class vm_handler_call : public base_command
    {
    public:
        explicit vm_handler_call(const call_type type)
            : base_command(command_type::vm_handler_call), call_type(type)
        {

        }

    private:
        call_type call_type = call_type::none;
    };
}
