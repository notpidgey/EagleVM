#pragma once
#include <array>
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_jmp final : public base_command
    {
        cmd_jmp();
    };
}