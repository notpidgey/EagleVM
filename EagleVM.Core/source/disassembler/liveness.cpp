#include "eaglevm-core/disassembler/liveness.h"

#include <ranges>
#include <unordered_set>

namespace eagle::dasm
{
    liveness_anal::liveness_anal(segment_dasm_ptr segment)
        : segment(std::move(segment))
    {
    }

    void liveness_anal::analyze(const basic_block_ptr& exit_block)
    {
        bool changed = true;
        while (changed)
        {
            changed = false;
            for (auto i = segment->blocks.size(); i--;)
            {
                const basic_block_ptr& block = segment->blocks[i];
                if (block == exit_block)
                    continue;

                auto search_jump = [this](const std::pair<uint64_t, block_jump_location> location) ->
                    std::optional<basic_block_ptr>
                {
                    if (location.second == jump_inside_segment)
                        return segment->get_block(location.first);

                    return std::nullopt;
                };

                // OUT[B]
                std::set<codec::reg> new_out;
                switch (block->get_end_reason())
                {
                    case block_conditional_jump:
                        if (auto search = search_jump(segment->get_jump(block, false)))
                            new_out.insert_range(in[search.value()]);
                    case block_end:
                    case block_jump:
                        if (auto search = search_jump(segment->get_jump(block, true)))
                            new_out.insert_range(in[search.value()]);
                        break;
                }

                // IN[B]
                auto [use_set, def_set] = block_use_def[block];

                std::set<codec::reg> diff;
                std::ranges::set_difference(new_out, def_set, std::inserter(diff, diff.begin()));

                std::set<codec::reg> new_in;
                new_in.insert_range(use_set);
                new_in.insert_range(diff);

                if (new_out != out[block] || new_in != in[block])
                    changed = true;

                in[block] = new_in;
                out[block] = new_out;
            }
        }

        // profit??
    }

    void liveness_anal::compute_use_def()
    {
        for (auto& block : segment->blocks)
        {
            std::set<codec::reg> use;
            std::set<codec::reg> def;

            for (auto [instruction, operands] : block->decoded_insts)
            {
                for (int i = 0; i < instruction.operand_count; i++)
                {
                    codec::dec::operand op = operands[i];
                    if (op.type == ZYDIS_OPERAND_TYPE_REGISTER)
                    {
                        if (op.reg.value == ZYDIS_REGISTER_RIP ||
                            op.reg.value == ZYDIS_REGISTER_EIP ||
                            op.reg.value == ZYDIS_REGISTER_FLAGS)
                            continue;

                        auto reg = get_bit_version(static_cast<codec::reg>(op.reg.value), codec::bit_64);
                        if (op.actions & ZYDIS_OPERAND_ACTION_MASK_READ && !def.contains(reg))
                            use.insert(reg);
                        if (op.actions & ZYDIS_OPERAND_ACTION_MASK_WRITE && !use.contains(reg))
                            def.insert(reg);
                    }
                    else if (op.type == ZYDIS_OPERAND_TYPE_MEMORY)
                    {
                        auto reg = get_bit_version(static_cast<codec::reg>(op.mem.base), codec::bit_64);
                        auto reg2 = get_bit_version(static_cast<codec::reg>(op.mem.index), codec::bit_64);

                        if (reg != ZYDIS_REGISTER_NONE && !def.contains(reg))
                            use.insert(reg);
                        if (reg2 != ZYDIS_REGISTER_NONE && !def.contains(reg2))
                            use.insert(reg2);
                    }
                }
            }

            block_use_def[block] = { use, def };
        }
    }
}
