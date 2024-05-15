#pragma once
#include <vector>
#include <string>
#include "eaglevm-core/codec/zydis_enum.h"

#include "eaglevm-core/virtual_machine/ir/models/il_size.h"
#include "eaglevm-core/virtual_machine/ir/x86/models/handler_op.h"

namespace eagle::ir
{
    using op_entries = std::vector<handler_op>;
    struct operand_signature
    {
        op_entries entries;
        std::string handler_id;
    };
}
