#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_context_store : public base_command
    {
    public:
        explicit cmd_context_store(const codec::reg dest)
            : base_command(command_type::vm_handler_call), dest(dest)
        {
        }

    private:
        codec::reg dest = codec::reg::none;
    };
}
