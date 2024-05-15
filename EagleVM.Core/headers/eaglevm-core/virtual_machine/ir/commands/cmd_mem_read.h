#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_stack.h"

namespace eagle::ir
{
    class cmd_mem_read : public base_command
    {
    public:
        explicit cmd_mem_read(il_size size);

        il_size get_read_size() const;

    private:
        il_size size;
    };
}
