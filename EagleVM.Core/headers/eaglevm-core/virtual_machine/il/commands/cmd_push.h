#pragma once
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"
#include "eaglevm-core/virtual_machine/il/models/il_reg.h"

namespace eagle::il
{
    class cmd_reg_push : public base_command
    {
    public:
        explicit cmd_reg_push(const reg_v reg_src, const reg_size reg_size)
            : base_command(command_type::vm_push), reg(reg_src), size(reg_size)
        {
            type = get_reg_type(reg_src);
        }

    private:
        reg_v reg;
        reg_type type;
        reg_size size;
    };
}
