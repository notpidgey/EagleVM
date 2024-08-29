#include "eaglevm-core/disassembler/analysis/liveness.h"
#include <ranges>
#include <unordered_set>

namespace eagle::dasm::analysis
{
    liveness::liveness(segment_dasm_ptr segment)
        : segment(std::move(segment))
    {
    }

    void liveness::analyze_cross_liveness(const basic_block_ptr& exit_block)
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

    std::vector<std::pair<reg_set, reg_set>> liveness::analyze_block_liveness(const basic_block_ptr& block)
    {
        std::vector<std::pair<reg_set, reg_set>> instruction_live(block->decoded_insts.size());

        bool changed = true;
        while (changed)
        {
            changed = false;
            for (auto i = block->decoded_insts.size(); i--;)
            {
                // OUT[B]
                std::set<codec::reg> new_out;
                if (i == block->decoded_insts.size() - 1)
                    new_out.insert_range(out[block]);
                else
                    new_out.insert_range(instruction_live[i + 1].first);

                // IN[B]
                std::set<codec::reg> use_set, def_set;
                compute_use_def(block->decoded_insts[i], use_set, def_set);

                std::set<codec::reg> diff;
                std::ranges::set_difference(new_out, def_set, std::inserter(diff, diff.begin()));

                std::set<codec::reg> new_in;
                new_in.insert_range(use_set);
                new_in.insert_range(diff);

                if (new_in != instruction_live[i].first || new_out != instruction_live[i].second)
                    changed = true;

                instruction_live[i] = { new_in, new_out };
            }
        }

        return instruction_live;
    }

    void liveness::compute_use_def()
    {
        for (auto& block : segment->blocks)
        {
            std::set<codec::reg> use;
            std::set<codec::reg> def;

            for (const auto& decoded_inst : block->decoded_insts)
                compute_use_def(decoded_inst, use, def);

            block_use_def[block] = { use, def };
        }
    }

    void liveness::compute_use_def(codec::dec::inst_info inst_info, reg_set& use, reg_set& def)
    {
        auto& [inst, operands] = inst_info;
        for (int i = 0; i < inst.operand_count; i++)
        {
            codec::dec::operand op = operands[i];
            auto handle_register = [&](codec::reg reg, const bool read)
            {
                // trying to write a lower 32 bit register
                // this means we have to clear the upper 32 bits which is another write
                if (!read && !use.contains(reg) && get_reg_class(reg) == codec::gpr_32)
                    def.insert(get_bit_version(reg, codec::bit_64));

                bool first = true;
                while (reg != ZYDIS_REGISTER_NONE)
                {
                    auto width = static_cast<uint16_t>(get_reg_size(reg));
                    if (width == 8 && !first)
                    {
                        // check for low, high
                        // we want to add the other part of the register
                        codec::reg reg_part = codec::reg::none;
                        if (reg >= ZYDIS_REGISTER_AL && reg <= ZYDIS_REGISTER_BL)
                            reg_part = static_cast<codec::reg>(static_cast<uint16_t>(reg + 4));
                        else if (reg >= ZYDIS_REGISTER_AH && reg <= ZYDIS_REGISTER_BH)
                            reg_part = static_cast<codec::reg>(static_cast<uint16_t>(reg - 4));

                        if (reg_part != codec::reg::none)
                        {
                            if (read && !def.contains(reg_part))
                                use.insert(reg_part);
                            if (!read && !use.contains(reg_part))
                                def.insert(reg_part);
                        }
                    }

                    // we wan
                    if (read && !def.contains(reg))
                    {
                        if (def.contains(reg))
                            break; // all the parts are going to exist already

                        use.insert(reg);
                    }
                    if (!read && !use.contains(reg))
                    {
                        if (use.contains(reg))
                            break; // all the parts are going to exist already

                        def.insert(reg);
                    }

                    if (reg == ZYDIS_REGISTER_RSP) break;

                    width /= 2;
                    reg = get_bit_version(reg, static_cast<codec::reg_size>(width));

                    first = false;
                }
            };

            if (op.type == ZYDIS_OPERAND_TYPE_REGISTER)
            {
                if (op.reg.value == ZYDIS_REGISTER_RIP ||
                    op.reg.value == ZYDIS_REGISTER_EIP ||
                    op.reg.value == ZYDIS_REGISTER_FLAGS)
                    continue;

                auto reg = static_cast<codec::reg>(op.reg.value);
                if (op.actions & ZYDIS_OPERAND_ACTION_MASK_READ && !def.contains(reg))
                    handle_register(reg, true);
                if (op.actions & ZYDIS_OPERAND_ACTION_MASK_WRITE && !use.contains(reg))
                    handle_register(reg, false);
            }
            else if (op.type == ZYDIS_OPERAND_TYPE_MEMORY)
            {
                codec::reg reg = static_cast<codec::reg>(op.mem.base);
                codec::reg reg2 = static_cast<codec::reg>(op.mem.index);

                if (reg != ZYDIS_REGISTER_NONE && !def.contains(reg))
                    handle_register(reg, true);
                if (reg2 != ZYDIS_REGISTER_NONE && !def.contains(reg2))
                    handle_register(reg2, true);
            }

            // if (inst.mnemonic == ZYDIS_MNEMONIC_CALL)
            // {
            //     // volatile regs
            //     auto volatile_regs = {
            //         ZYDIS_REGISTER_RAX,
            //         ZYDIS_REGISTER_RCX,
            //         ZYDIS_REGISTER_RDX,

            //         ZYDIS_REGISTER_R8,
            //         ZYDIS_REGISTER_R9,
            //         ZYDIS_REGISTER_R10,
            //         ZYDIS_REGISTER_R11,
            //     };

            //     for (auto& reg : volatile_regs)
            //     {
            //         handle_register(static_cast<codec::reg>(reg), true);
            //         handle_register(static_cast<codec::reg>(reg), false);
            //     }
            // }
        }
    }
}
