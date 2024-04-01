#pragma once
#include <vector>
#include "eaglevm-core/virtual_machine/il/models/il_size.h"

namespace eagle::il
{
    struct handler_info
    {
        il_size instruction_width = qword;
        uint8_t operand_count = 0;
    };

    using handler_entries = std::vector<handler_info>;
}
