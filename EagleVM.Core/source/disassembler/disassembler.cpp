#include "eaglevm-core/disassembler/disassembler.h"

segment_dasm::segment_dasm(const decode_vec& segment, const uint32_t binary_rva, const uint32_t binary_end)
    : root_block(nullptr)
{
    function = segment;
    rva_begin = binary_rva;
    rva_end = binary_end;
}

basic_block* segment_dasm::generate_blocks()
{
    uint32_t block_start_rva = rva_begin;
    uint32_t current_rva = rva_begin;

    decode_vec block_instructions;
    for (auto& inst: function)
    {
        block_instructions.push_back(inst);

        if (inst.instruction.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE &&
            inst.instruction.mnemonic != ZYDIS_MNEMONIC_CALL)
        {
            // end of our block
            basic_block* block = new basic_block();
            block->start_rva = block_start_rva;
            block->end_rva_inc = current_rva + inst.instruction.length;
            block->decoded_insts = block_instructions;

            blocks.push_back(block);
            block_instructions.clear();

            block_start_rva = block->end_rva_inc;
        }

        current_rva += inst.instruction.length;
    }

    if (!block_instructions.empty())
    {
        basic_block* block = new basic_block();
        block->start_rva = block_start_rva;
        block->end_rva_inc = current_rva;
        block->decoded_insts = block_instructions;

        blocks.push_back(block);
    }

    // but here we have an issue, some block may be jumping inside of another block
    // we need to fix that
    for (int i = 0; i < blocks.size(); i++)
    {
        // we only care about jumping blocks inside of the segment
        const basic_block* block = blocks[i];
        if (block->get_end_reason() == block_end)
            continue;

        const auto& [jump_rva, jump_type] = get_jump(block);
        if (jump_type != jump_inside_segment)
            continue;

        basic_block* new_block = nullptr;
        for (basic_block* target_block: blocks)
        {
            // non inclusive is key because we might already be at that block
            if (jump_rva <= target_block->start_rva || jump_rva >= target_block->end_rva_inc)
                continue;

            // we found a jump to the middle of a block
            // we need to split the block
            basic_block* split_block = new basic_block();
            split_block->start_rva = jump_rva;
            split_block->end_rva_inc = target_block->end_rva_inc;
            target_block->end_rva_inc = jump_rva;

            // for the new_block, we copy all the instructions starting at the far_rva
            uint32_t curr_rva = target_block->start_rva;
            for (int j = 0; j < target_block->decoded_insts.size();)
            {
                zydis_decode& inst = target_block->decoded_insts[j];
                if (curr_rva >= jump_rva)
                {
                    // add to new block, remove from old block
                    split_block->decoded_insts.push_back(inst);
                    target_block->decoded_insts.erase(target_block->decoded_insts.begin() + j);
                }
                else
                {
                    j++;
                }

                curr_rva += inst.instruction.length; // move this line up
            }

            new_block = split_block;
            break;
        }

        if (new_block)
            blocks.push_back(new_block);
    }


    std::ranges::sort(blocks,
        [](const basic_block* a, const basic_block* b)
        {
            return a->start_rva < b->start_rva;
        });

    return blocks[0];
}

std::pair<uint64_t, block_jump_location> segment_dasm::get_jump(const basic_block* block, bool last)
{
    block_end_reason end_reason = block->get_end_reason();
    if(last)
        end_reason = block_end;

    switch (end_reason)
    {
        case block_end:
        {
            return {block->end_rva_inc, get_jump_location(block->end_rva_inc)};
        }
        case block_jump:
        {
            zydis_decode last_inst = block->decoded_insts.back();
            const uint64_t last_inst_rva = block->end_rva_inc - last_inst.instruction.length;

            auto [target_rva, _] = zydis_helper::calc_relative_rva(last_inst, last_inst_rva);
            return {target_rva, get_jump_location(target_rva)};
        }
        case block_conditional_jump:
        {
            zydis_decode last_inst = block->decoded_insts.back();
            const uint64_t last_inst_rva = block->end_rva_inc - last_inst.instruction.length;

            auto [target_rva, _] = zydis_helper::calc_relative_rva(last_inst, last_inst_rva);
            return {target_rva, get_jump_location(target_rva)};
        }
    }
}

block_jump_location segment_dasm::get_jump_location(const uint32_t rva) const
{
    if (rva >= rva_begin && rva < rva_end)
        return jump_inside_segment;

    return jump_outside_segment;
}

basic_block* segment_dasm::get_block(const uint32_t rva) const
{
    for (basic_block* block: blocks)
    {
        if (block->start_rva <= rva && rva < block->end_rva_inc)
            return block;
    }

    return nullptr;
}
