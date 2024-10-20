#pragma once
#include "eaglevm-core/virtual_machine/ir/x86/base_x86_translator.h"

namespace eagle::ir::handler::util
{
   uint8_t flag_index(uint64_t mask);

   ir_insts calculate_sf(ir_size size, const discrete_store_ptr& result);
   ir_insts calculate_zf(ir_size size, const discrete_store_ptr& result);
   ir_insts calculate_pf(ir_size size, const discrete_store_ptr& result);

   enum top_arg
   {
      param_one,
      param_two,
      result,
   };

   ir_insts copy_to_top(ir_size size, top_arg target_arg, uint8_t byte_offset = 0);
}
