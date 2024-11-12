#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/cmd_logic.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_mem_read.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_push.h"
#include "eaglevm-core/virtual_machine/ir/x86/base_x86_translator.h"

namespace eagle::ir::handler::util
{
    uint8_t flag_index(uint64_t mask);

    ir_insts calculate_sf(ir_size size);
    ir_insts calculate_zf(ir_size size);
    ir_insts calculate_pf(ir_size size);

    enum top_arg
    {
        param_one,
        param_two,
        result,
    };

    inline ir_insts copy_to_top(ir_size size, const top_arg target_arg, const std::initializer_list<ir_size> offsets = {})
    {
        uint32_t total_offset = static_cast<uint32_t>(ir_size::bit_64); // rflags
        for (const ir_size offset : offsets)
            total_offset += static_cast<uint32_t>(offset);

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

        return ir_insts {
            std::make_shared<cmd_push>(reg_vm::vsp, ir_size::bit_64),
            std::make_shared<cmd_push>(total_offset / 8, ir_size::bit_64),
            std::make_shared<cmd_add>(ir_size::bit_64),
            std::make_shared<cmd_mem_read>(size),
        };
    }
}
