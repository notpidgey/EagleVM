#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_context_store : public base_command
    {
    public:
        explicit cmd_context_store(codec::reg dest);
        codec::reg get_reg() const;

    private:
        codec::reg dest;
    };
}
