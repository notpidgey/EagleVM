#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_load.h"
#include "eaglevm-core/codec/zydis_helper.h"

namespace eagle::ir
{
    cmd_context_load::cmd_context_load(const codec::reg source)
        : base_command(command_type::vm_context_load),
          source(source), r_class(codec::get_reg_class(source))
    {

    }

    codec::reg cmd_context_load::get_reg() const
    {
        return source;
    }

    codec::reg_class cmd_context_load::get_reg_class() const
    {
        return r_class;
    }
}
