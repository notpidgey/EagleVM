#pragma once
#include <vector>
#include <string>
#include "eaglevm-core/codec/zydis_enum.h"

#include "eaglevm-core/virtual_machine/ir/models/ir_size.h"
#include "eaglevm-core/virtual_machine/ir/x86/models/handler_op.h"
#include "eaglevm-core/virtual_machine/ir/x86/models/hash_string.h"

namespace eagle::ir
{
    using op_params = std::vector<handler_op>;
    struct op_signature
    {
        op_params entries;
        uint64_t handler_id;

        op_signature(const op_params& entries, const std::string_view view)
        {
            this->entries = entries;
            handler_id = hash_string::hash(view);
        }
    };
}
