#pragma once
#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/codec/zydis_enum.h"
#include "eaglevm-core/codec/zydis_helper.h"

#include "eaglevm-core/virtual_machine/il/commands/include.h"
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"
#include "eaglevm-core/virtual_machine/il/x86/models/handler_info.h"

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
