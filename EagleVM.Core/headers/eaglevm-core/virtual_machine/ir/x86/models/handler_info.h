#pragma once
#include <vector>
#include "eaglevm-core/codec/zydis_enum.h"

#include "eaglevm-core/virtual_machine/ir/models/il_size.h"
#include "eaglevm-core/virtual_machine/ir/x86/models/handler_op.h"

namespace eagle::ir
{
    using op_entries = std::vector<handler_op>;
    struct handler_info
    {
        op_entries entries;
    };
}
