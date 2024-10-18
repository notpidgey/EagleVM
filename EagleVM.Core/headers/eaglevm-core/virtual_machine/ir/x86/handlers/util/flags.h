#pragma once
#include "eaglevm-core/virtual_machine/ir/x86/base_x86_translator.h"

namespace eagle::ir::handler::util
{
   uint8_t flag_index(uint64_t mask);

   ir_insts calculate_sf(ir_size size, const discrete_store_ptr& flags, const discrete_store_ptr& result, bool restore = true);
   ir_insts calculate_zf(ir_size size, const discrete_store_ptr& flags, const discrete_store_ptr& result, bool restore = true);
   ir_insts calculate_pf(ir_size size, const discrete_store_ptr& flags, const discrete_store_ptr& result, bool restore = true);
}
