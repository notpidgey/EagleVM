#pragma once
#include <optional>

#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/codec/zydis_enum.h"
#include "eaglevm-core/codec/zydis_helper.h"

#include "eaglevm-core/virtual_machine/ir/commands/include.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/x86/models/handler_info.h"

#include "eaglevm-core/virtual_machine/ir/x86/models/handler_op.h"

namespace eagle::il::handler
{
    using handler_info_ptr = std::shared_ptr<handler_info>;
    class base_handler_gen
    {
    public:
        base_handler_gen()
        {
            handlers = {};
        }

        virtual il_insts gen_handler(codec::reg_class size, uint8_t operands) = 0;
        [[nodiscard]] handler_info_ptr get_operand_handler(const std::vector<handler_op>& target_operands) const;

    protected:
        ~base_handler_gen() = default;
        std::vector<handler_info_ptr> handlers;
    };

    using base_handler_gen_ptr = std::shared_ptr<base_handler_gen>;
    using gen_info_pair = std::pair<base_handler_gen_ptr, handler_info_ptr>;
}
