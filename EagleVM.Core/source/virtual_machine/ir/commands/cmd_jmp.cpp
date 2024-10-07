#include "eaglevm-core/virtual_machine/ir/commands/cmd_jmp.h"

namespace eagle::ir
{
    cmd_jmp::cmd_jmp()
        : base_command(command_type::vm_jmp)
    {
    }
}
