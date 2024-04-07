#pragma once
#include <vector>
#include "eaglevm-core/codec/zydis_enum.h"
#include "eaglevm-core/virtual_machine/il/models/il_size.h"

namespace eagle::il
{
    struct handler_info
    {
        codec::reg_class instruction_width = codec::reg_class::invalid;
        uint8_t operand_count = 0;
    };

    using handler_entries = std::vector<handler_info>;
}
