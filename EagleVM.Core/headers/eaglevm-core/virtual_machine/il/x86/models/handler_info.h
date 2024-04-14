#pragma once
#include <vector>
#include "eaglevm-core/codec/zydis_enum.h"

#include "eaglevm-core/virtual_machine/il/models/il_size.h"
#include "eaglevm-core/virtual_machine/il/x86/models/handler_op.h"

namespace eagle::il
{
    using handler_entries = std::vector<std::vector<handler_op>>;
}
