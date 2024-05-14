#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_pop : public base_command
    {
    public:
        explicit cmd_pop(const reg_vm reg_dest, const il_size size)
            : base_command(command_type::vm_pop), dest_reg(reg_dest), size(size)
        {
            type = get_reg_type(reg_dest);
        }

    private:
        reg_vm dest_reg;
        reg_type type;
        il_size size;
    };
}
