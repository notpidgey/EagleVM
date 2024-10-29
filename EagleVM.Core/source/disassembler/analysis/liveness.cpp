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

                auto search_jump = [this](const std::pair<uint64_t, block_jump_location> location) ->
                    std::optional<basic_block_ptr>
                {
                    if (location.second == jump_inside_segment)
                        return segment->get_block(location.first);

                    return std::nullopt;
                };

                // OUT[B]
                liveness_info new_out = { };
                switch (block->get_end_reason())
                {
                    case block_conditional_jump:
                        if (auto search = search_jump(segment->get_jump(block, false)))
                            new_out |= live[search.value()].first;
                    case block_end:
                    case block_jump:
                        if (auto search = search_jump(segment->get_jump(block, true)))
                            new_out |= live[search.value()].first;
                        break;
                }

                // IN[B]
                auto [use_set, def_set] = block_use_def[block];

                const liveness_info diff = new_out - def_set;
                liveness_info new_in = use_set | diff;

                if (new_out != live[block].second || new_in != live[block].first)
                    changed = true;

                live[block].first = new_in;
                live[block].second = new_out;
            }
        }

        // profit??
    }

    std::vector<std::pair<liveness_info, liveness_info>> liveness::analyze_block(const basic_block_ptr& block)
    {
        std::vector<std::pair<liveness_info, liveness_info>> instruction_live(block->decoded_insts.size());

        bool changed = true;
        while (changed)
        {
            changed = false;
            for (auto i = block->decoded_insts.size(); i--;)
            {
                // OUT[B]
                liveness_info new_out;
                if (i == block->decoded_insts.size() - 1) new_out = live[block].second;
                else new_out = instruction_live[i + 1].first;

                // IN[B]
                liveness_info use_set, def_set;
                compute_inst_use_def(block->decoded_insts[i], use_set, def_set);

                const liveness_info diff = new_out - def_set;
                liveness_info new_in = use_set | diff;

                if (new_in != instruction_live[i].first || new_out != instruction_live[i].second)
                    changed = true;

                instruction_live[i] = { new_in, new_out };
            }
        }

        return instruction_live;
    }

    std::pair<liveness_info, liveness_info> liveness::analyze_block_at(const basic_block_ptr& block, const size_t idx)
    {
        const std::vector<std::pair<liveness_info, liveness_info>> instruction_live
            = analyze_block(block);

        return instruction_live[idx];
    }

    void liveness::compute_blocks_use_def()
    {
        for (auto& block : segment->blocks)
        {
            liveness_info use, def = { };
            for (const auto& decoded_inst : block->decoded_insts)
                compute_inst_use_def(decoded_inst, use, def);

            block_use_def[block] = { use, def };
        }
    }

    liveness_info liveness::compute_inst_flags(const codec::dec::inst_info& inst_info)
    {
        liveness_info liveness = { };
        liveness.insert_flags(inst_info.instruction.cpu_flags->modified);
        liveness.insert_flags(inst_info.instruction.cpu_flags->set_0);
        liveness.insert_flags(inst_info.instruction.cpu_flags->set_1);

        return liveness;
    }

    void liveness::compute_inst_use_def(codec::dec::inst_info inst_info, liveness_info& use, liveness_info& def)
    {
        auto& [inst, operands] = inst_info;
        auto handle_register = [&](codec::reg reg, const bool read)
        {
            // trying to write a lower 32 bit register
            // this means we have to clear the upper 32 bits which is another write
            if (!read && get_reg_class(reg) == codec::gpr_32)
                def.insert_register(get_bit_version(reg, codec::bit_64));

            if (read) use.insert_register(reg);
            if (!read) def.insert_register(reg);
        };

        if (inst.mnemonic == ZYDIS_MNEMONIC_CALL)
        {
            // massive assumption that the calling conventions are perfect
            auto read_volatile_regs = {
                ZYDIS_REGISTER_RCX,
                ZYDIS_REGISTER_RDX,

                ZYDIS_REGISTER_R8,
                ZYDIS_REGISTER_R9,

                ZYDIS_REGISTER_RSP
            };

            for (auto& reg : read_volatile_regs)
                handle_register(static_cast<codec::reg>(reg), true);

            auto written_volatile_regs = {
                ZYDIS_REGISTER_RAX,
                ZYDIS_REGISTER_RCX,
                ZYDIS_REGISTER_RDX,

                ZYDIS_REGISTER_R8,
                ZYDIS_REGISTER_R9,
                ZYDIS_REGISTER_R10,
                ZYDIS_REGISTER_R11,
            };

            for (auto& reg : written_volatile_regs)
                handle_register(static_cast<codec::reg>(reg), false);
        }

        for (int i = 0; i < inst.operand_count; i++)
        {
            codec::dec::operand op = operands[i];
            if (op.type == ZYDIS_OPERAND_TYPE_REGISTER)
            {
                if (op.reg.value == ZYDIS_REGISTER_RIP ||
                    op.reg.value == ZYDIS_REGISTER_EIP)
                    continue;

                const auto reg = static_cast<codec::reg>(op.reg.value);
                if (op.actions & ZYDIS_OPERAND_ACTION_MASK_READ)
                    handle_register(reg, true);
                if (op.actions & ZYDIS_OPERAND_ACTION_MASK_WRITE)
                    handle_register(reg, false);
            }
            else if (op.type == ZYDIS_OPERAND_TYPE_MEMORY)
            {
                const codec::reg reg = static_cast<codec::reg>(op.mem.base);
                const codec::reg reg2 = static_cast<codec::reg>(op.mem.index);

                if (reg != ZYDIS_REGISTER_NONE)
                    handle_register(reg, true);
                if (reg2 != ZYDIS_REGISTER_NONE)
                    handle_register(reg2, true);
            }

            // check read flags
            use.insert_flags(inst.cpu_flags->tested);

            // check written flags
            def.insert_flags(inst.cpu_flags->modified);
            def.insert_flags(inst.cpu_flags->set_0);
            def.insert_flags(inst.cpu_flags->set_1);
        }
    }
}
