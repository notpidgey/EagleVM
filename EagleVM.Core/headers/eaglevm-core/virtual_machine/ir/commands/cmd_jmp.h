#pragma once
#include <array>
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_jmp : public base_command
    {
    public:
        cmd_jmp();

        BASE_COMMAND_CLONE(cmd_jmp);
    };

    SHARED_DEFINE(cmd_jmp);
}