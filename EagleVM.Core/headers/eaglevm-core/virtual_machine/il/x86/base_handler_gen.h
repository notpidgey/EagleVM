#pragma once
#include <optional>

#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/codec/zydis_enum.h"
#include "eaglevm-core/codec/zydis_helper.h"

#include "eaglevm-core/virtual_machine/il/commands/include.h"
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"
#include "eaglevm-core/virtual_machine/il/x86/models/handler_info.h"

#include "eaglevm-core/virtual_machine/il/x86/models/handler_op.h"

namespace eagle::il::handler
{
    class base_handler_gen
    {
    public:
        base_handler_gen()
        {
            entries = {};
        }

        virtual il_insts gen_handler(codec::reg_class size, uint8_t operands) = 0;
        bool get_handler(const std::vector<handler_op>& target_operands) const;

    protected:
        ~base_handler_gen() = default;
        handler_entries entries;
    };
}
