#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_store.h"

namespace eagle::ir
{
    cmd_context_store::cmd_context_store(const codec::reg dest)
        : base_command(command_type::vm_context_store), dest(dest)
    {
    }

    codec::reg cmd_context_store::get_reg() const
    {
        return dest;
    }
}
