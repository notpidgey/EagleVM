#pragma once
#include <cassert>

#include "eaglevm-core/util/assert.h"
#include "eaglevm-core/virtual_machine/ir/models/ir_size.h"

namespace eagle::ir
{
    inline ir_size bits_to_ir_size(const uint16_t bit_count);
}
