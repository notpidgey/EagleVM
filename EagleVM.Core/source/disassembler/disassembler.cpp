#include "eaglevm-core/disassembler/disassembler.h"

namespace eagle::dasm
{
    segment_dasm::segment_dasm(const codec::decode_vec& segment, const uint64_t binary_rva, const uint64_t binary_end)
        : root_block(nullptr)
    {
        function = segment;

        rva_begin = binary_rva;
        rva_end = binary_end;
    }

    basic_block_ptr segment_dasm::generate_blocks()
    {
        uint64_t block_start_rva = rva_begin;
        uint64_t current_rva = rva_begin;

        codec::decode_vec block_instructions;
        for (auto& inst : function)
        {
            block_instructions.push_back(inst);

            if (inst.instruction.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE &&
                inst.instruction.mnemonic != ZYDIS_MNEMONIC_CALL)
            {
                // end of our block
                basic_block_ptr block = std::make_shared<basic_block>();
                block->start_rva = block_start_rva;
                block->end_rva_inc = current_rva + inst.instruction.length;
                block->decoded_insts = block_instructions;

                block_instructions.clear();
                block_start_rva = block->end_rva_inc;

                blocks.push_back(std::move(block));
            }

            current_rva += inst.instruction.length;
        }

        if (!block_instructions.empty())
        {
            basic_block_ptr block = std::make_shared<basic_block>();
            block->start_rva = block_start_rva;
            block->end_rva_inc = current_rva;
            block->decoded_insts = block_instructions;

            blocks.push_back(std::move(block));
        }

        // but here we have an issue, some block may be jumping inside of another block
        // we need to fix that
        std::vector<basic_block_ptr> new_blocks;

        for (auto i = 0; i < blocks.size(); i++)
        {
            const basic_block_ptr &block = blocks[i];
            if (block->get_end_reason() == block_end)
                continue;

            const auto &[jump_rva, jump_type] = get_jump(block);
            if (jump_type != jump_inside_segment)
                continue;

            for (auto j = 0; j < blocks.size(); j++)
            {
                const basic_block_ptr &target_block = blocks[j];
                if (jump_rva <= target_block->start_rva || jump_rva >= target_block->end_rva_inc)
                    continue;

                basic_block_ptr split_block = std::make_shared<basic_block>();
                split_block->start_rva = jump_rva;
                split_block->end_rva_inc = target_block->end_rva_inc;
                target_block->end_rva_inc = jump_rva;

                uint64_t curr_rva = target_block->start_rva;
                for (auto k = 0; k < target_block->decoded_insts.size();)
                {
                    codec::dec::inst_info &inst = target_block->decoded_insts[k];
                    if (curr_rva >= jump_rva)
                    {
                        split_block->decoded_insts.push_back(inst);
                        target_block->decoded_insts.erase(target_block->decoded_insts.begin() + k);
                    }
                    else
                    {
                        k++;
                    }
                    curr_rva += inst.instruction.length;
                }

                new_blocks.push_back(std::move(split_block));
                break;
            }
        }

        for (auto &new_block : new_blocks)
            blocks.push_back(std::move(new_block));

        std::ranges::sort(blocks, [](auto a, auto b)
        {
            return a->start_rva < b->start_rva;
        });

        return blocks[0];
    }

    std::pair<uint64_t, block_jump_location> segment_dasm::get_jump(const basic_block_ptr block, bool last)
    {
        block_end_reason end_reason = block->get_end_reason();
        if (last)
            end_reason = block_end;

        switch (end_reason)
        {
            case block_end:
            {
                return { block->end_rva_inc, get_jump_location(block->end_rva_inc) };
            }
            case block_jump:
            {
                auto last_inst = block->decoded_insts.back();
                const uint64_t last_inst_rva = block->end_rva_inc - last_inst.instruction.length;

                auto [target_rva, _] = codec::calc_relative_rva(last_inst, last_inst_rva);
                return { target_rva, get_jump_location(target_rva) };
            }
            case block_conditional_jump:
            {
                auto last_inst = block->decoded_insts.back();
                const uint64_t last_inst_rva = block->end_rva_inc - last_inst.instruction.length;

                auto [target_rva, _] = codec::calc_relative_rva(last_inst, last_inst_rva);
                return { target_rva, get_jump_location(target_rva) };
            }
        }

        return { 0, jump_unknown };
    }

    block_jump_location segment_dasm::get_jump_location(const uint64_t rva) const
    {
        if (rva >= rva_begin && rva < rva_end)
            return jump_inside_segment;

        return jump_outside_segment;
    }

    basic_block_ptr segment_dasm::get_block(const uint64_t rva) const
    {
        for (basic_block_ptr block : blocks)
        {
            if (block->start_rva <= rva && rva < block->end_rva_inc)
                return block;
        }

        return nullptr;
    }
}
