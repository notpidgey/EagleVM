#pragma once
#include "eaglevm-core/virtual_machine/ir/block.h"

namespace eagle::ir::analyzer
{
    bool should_release(discrete_store_ptr store, const block_ptr& block, size_t idx);
}
