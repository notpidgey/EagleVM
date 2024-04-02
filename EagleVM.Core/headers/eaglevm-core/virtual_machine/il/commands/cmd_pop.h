#pragma once
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"

namespace eagle::il
{
    class cmd_reg_pop : public base_command
    {
    public:
        explicit cmd_reg_pop(const reg_v reg_dest, const reg_size size)
            : base_command(command_type::vm_pop), dest_reg(reg_dest), size(size)
        {
            type = get_reg_type(reg_dest);
        }

    private:
        reg_v dest_reg;
        reg_type type;
        reg_size size;
    };
}
