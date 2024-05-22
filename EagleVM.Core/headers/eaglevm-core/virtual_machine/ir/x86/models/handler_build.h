#pragma once
#include <vector>
#include <string>

#include "eaglevm-core/virtual_machine/ir/models/ir_size.h"

namespace eagle::ir
{
    struct handler_build
    {
        std::vector<ir_size> size;
        std::string handler_id;
    };
}