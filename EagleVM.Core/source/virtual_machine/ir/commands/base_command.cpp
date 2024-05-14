#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    command_type base_command::get_command_type()
    {
        return command;
    }
}
