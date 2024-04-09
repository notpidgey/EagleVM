#pragma once
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"
#include "eaglevm-core/virtual_machine/il/commands/models/cmd_stack.h"

namespace eagle::il
{
    class cmd_mem_write : public base_command
    {
    public:
        explicit cmd_mem_write(const reg_vm reg_dest, const reg_vm reg_source, const il_size size)
            : base_command(command_type::vm_mem_write), dest_reg(reg_dest), source_reg(reg_source), size(size)
        {
        }

    private:
        reg_vm dest_reg;
        reg_vm source_reg;
        il_size size;
    };
}
