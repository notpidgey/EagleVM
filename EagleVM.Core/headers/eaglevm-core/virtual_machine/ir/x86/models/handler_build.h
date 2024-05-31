#pragma once
#include <vector>
#include <string>

#include "eaglevm-core/virtual_machine/ir/models/ir_size.h"

namespace eagle::ir
{
    using handler_params = std::vector<ir_size>;
    struct handler_build
    {
        handler_params params;
        std::string handler_id;
    };
}