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

    ir_insts copy_to_top(ir_size size, const top_arg target_arg, ir_size offsets...)
    {
        // this is how the stack should look like

        // 0x0: param two
        // 0x(size * 1): para one
        // 0x(size * 2): result
        // 0x(size * 3 + bit_64): rflags

        uint32_t total_offset = static_cast<uint32_t>(ir_size::bit_64);
        total_offset += static_cast<uint32_t>(ir_size::bit_64);

        switch (target_arg)
        {
            case param_two:
                total_offset += 2 * static_cast<uint32_t>(size);
                break;
            case param_one:
                total_offset += static_cast<uint32_t>(size);
                break;
            case result:
                break;
        }

        uint32_t additional_offsets[] = { static_cast<uint32_t>(offsets)... };
        for (const uint32_t offset : additional_offsets)
            total_offset += offset;

        return ir_insts(total_offset);
    }
}
