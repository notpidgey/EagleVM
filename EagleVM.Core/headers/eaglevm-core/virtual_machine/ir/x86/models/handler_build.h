#pragma once
#include <vector>
#include <string>

#include "eaglevm-core/virtual_machine/ir/models/ir_size.h"
#include "eaglevm-core/virtual_machine/ir/x86/models/hash_string.h"

namespace eagle::ir
{
    using handler_params = std::vector<ir_size>;
    struct handler_build
    {
        handler_params params;
        uint64_t handler_id;

        handler_build(const handler_params& params, const std::string_view view)
        {
            this->params = params;
            handler_id = hash_string::hash(view);
        }
    };
}