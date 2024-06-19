#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_store.h"
#include "eaglevm-core/codec/zydis_helper.h"

namespace eagle::ir
{
    cmd_context_store::cmd_context_store(const codec::reg dest)
        : base_command(command_type::vm_context_store), dest(dest), size(get_reg_size(dest))
    {
    }

    cmd_context_store::cmd_context_store(const codec::reg dest, const codec::reg_size size)
    : base_command(command_type::vm_context_store), dest(dest), size(size)
    {
    }

    codec::reg cmd_context_store::get_reg() const
    {
        return dest;
    }

    codec::reg_size cmd_context_store::get_value_size() const
    {
        return size;
    }
}
