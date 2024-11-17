#pragma once
#include <optional>

#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/codec/zydis_enum.h"
#include "eaglevm-core/codec/zydis_helper.h"

#include "eaglevm-core/virtual_machine/ir/commands/include.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

#include "eaglevm-core/virtual_machine/ir/x86/models/op_signature.h"
#include "eaglevm-core/virtual_machine/ir/x86/models/handler_op.h"
#include "eaglevm-core/virtual_machine/ir/x86/models/handler_build.h"

namespace eagle::ir::handler
{
    class base_handler_gen
    {
    public:
        base_handler_gen()
            : valid_operands({ })
        {
        }

        virtual ir_insts gen_handler(uint64_t target_handler_id);
        virtual ir_insts gen_handler(handler_sig signature);

        [[nodiscard]] std::optional<uint64_t> get_handler_id(const op_params& target_operands);
        [[nodiscard]] std::optional<uint64_t> get_handler_id(const handler_sig& target_build);
        [[nodiscard]] std::optional<handler_build> get_handler_build(uint64_t target_handler_id) const;

    protected:
        ~base_handler_gen() = default;
        std::vector<op_signature> valid_operands;
        std::vector<handler_build> build_options;
    };

    using base_handler_gen_ptr = std::shared_ptr<base_handler_gen>;
    using gen_info_pair = std::pair<base_handler_gen_ptr, op_signature>;
}
