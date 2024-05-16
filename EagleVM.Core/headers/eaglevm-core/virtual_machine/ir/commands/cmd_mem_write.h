#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_stack.h"

namespace eagle::ir
{
    class cmd_mem_write : public base_command
    {
    public:
        explicit cmd_mem_write(ir_size value_size, ir_size write_size);

        ir_size get_value_size() const;
        ir_size get_write_size() const;

    private:
        ir_size write_size;
        ir_size value_size;
    };
}
