#include "disassembler/disassembler.h"

segment_dasm::segment_dasm(const decode_vec& segment, const uint32_t binary_rva, const uint32_t binary_end)
    : root_block(nullptr)
{
    function = segment;
    rva_begin = binary_rva;
    rva_end = binary_end;
}

void segment_dasm::generate_blocks()
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
        zydis_decode& inst = function.back();

        basic_block* block = new basic_block();
        block->start_rva = block_start_rva;
        block->end_rva_inc = current_rva;
        block->decoded_insts = block_instructions;

        blocks.push_back(block);
    }

    // but here we have an issue, some block may be jumping inside of another block
    // we need to fix that
    std::vector<basic_block *> new_blocks;

    for (const basic_block* block: blocks)
    {
        // we only care about jumping blocks
        if (block->get_end_reason() == block_end)
            continue;

        const auto& [jump_rva, jump_type] = get_jump(block);
        for (basic_block* target_block: blocks)
        {
            // non inclusive is key because we might already be at that block
            if (jump_rva > target_block->start_rva && jump_rva < target_block->end_rva_inc)
            {
                // we found a jump to the middle of a block
                // we need to split the block

                basic_block* new_block = new basic_block();
                new_block->start_rva = jump_rva;
                new_block->end_rva_inc = target_block->end_rva_inc;
                target_block->end_rva_inc = jump_rva;

                block_jump_location location = jump_rva > rva_end || jump_rva < rva_begin
                                                   ? jump_outside_segment
                                                   : jump_inside_segment;
                target_block->end_rva_inc = jump_rva;

                // for the new_block, we copy all the instructions starting at the far_rva
                uint32_t curr_rva = target_block->start_rva;
                for (int i = 0; i < target_block->decoded_insts.size();)
                {
                    zydis_decode& inst = target_block->decoded_insts[i];
                    if (curr_rva >= jump_rva)
                    {
                        // add to new block, remove from old block
                        new_block->decoded_insts.push_back(inst);
                        target_block->decoded_insts.erase(target_block->decoded_insts.begin() + i);
                    }
                    else
                    {
                        ++i;
                    }

                    curr_rva += inst.instruction.length; // move this line up
                }

                new_blocks.push_back(new_block);
            }
        }
    }

    blocks.insert(blocks.end(), new_blocks.begin(), new_blocks.end());
}

std::pair<uint64_t, block_jump_location> segment_dasm::get_jump(const basic_block* block, bool last)
{
    const block_end_reason end_reason = block->get_end_reason();
    switch (end_reason)
    {
        case block_end:
        {
            return {block->end_rva_inc, get_jump_location(block->end_rva_inc)};
        }
        case block_jump || !last:
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
        if (block->start_rva >= rva && rva < block->end_rva_inc)
            return block;
    }

    return nullptr;
}
