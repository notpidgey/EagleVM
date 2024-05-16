#pragma once
#include <utility>

#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_pop : public base_command
    {
    public:
        explicit cmd_pop(discrete_store_ptr  reg_dest, const ir_size size)
            : base_command(command_type::vm_pop), dest_reg(std::move(reg_dest)), size(size)
        {
        }

    private:
        discrete_store_ptr dest_reg;
        ir_size size;
    };
}
