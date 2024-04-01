#pragma once
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"
#include "eaglevm-core/virtual_machine/il/handlers/models/handler_info.h"

#include "eaglevm-core/virtual_machine/il/commands/include.h"

namespace eagle::il::handlers
{
    class base_handler_gen
    {
    public:
        base_handler_gen()
        {
            entries = {};
        }

        virtual il_insts gen_il(reg_size size, uint8_t operands) = 0;

    protected:
        ~base_handler_gen() = default;

    private:
        handler_entries entries;
    };
}
