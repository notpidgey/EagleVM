#pragma once
#include <vector>
#include "eaglevm-core/codec/zydis_enum.h"

#include "eaglevm-core/virtual_machine/il/models/il_size.h"
#include "eaglevm-core/virtual_machine/il/x86/models/handler_op.h"

namespace eagle::il
{
    using op_entries = std::vector<handler_op>;
    struct handler_info
    {
        op_entries entries;
    };
}
