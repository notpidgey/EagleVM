#pragma once
#include "eaglevm-core/virtual_machine/il/commands/include.h"
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"
#include "models/handler_info.h"

namespace eagle::il::handler
{
    class base_handler_gen
    {
    public:
        base_handler_gen()
        {
            entries = {};
        }

        virtual il_insts gen_il(codec::reg_class size, uint8_t operands) = 0;

    protected:
        ~base_handler_gen() = default;
        handler_entries entries;
    };
}
