#pragma once
#include <optional>
#include "eaglevm-core/virtual_machine/ir/commands/cmd_branch.h"

namespace eagle::ir
{
    struct branch_info
    {
        std::optional<ir_exit_result> fallthrough_branch;
        std::optional<ir_exit_result> conditional_branch;

        exit_condition exit_condition;
        bool inverted_condition;
    };
}
