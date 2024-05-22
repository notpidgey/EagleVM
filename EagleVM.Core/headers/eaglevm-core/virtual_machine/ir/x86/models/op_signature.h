#pragma once
#include <vector>
#include <string>
#include "eaglevm-core/codec/zydis_enum.h"

#include "eaglevm-core/virtual_machine/ir/models/ir_size.h"
#include "eaglevm-core/virtual_machine/ir/x86/models/handler_op.h"

namespace eagle::ir
{
    using op_entries = std::vector<handler_op>;
    struct op_signature
    {
        op_entries entries;
        std::string handler_id;
    };
}
