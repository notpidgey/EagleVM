#include "disassembler/disassembler.h"

segment_disassembler::segment_disassembler(const decode_vec& segment, const uint32_t binary_rva,
                                           const uint32_t binary_end) : root_block(nullptr)
{
    function = segment;
    rva_begin = binary_rva;
    rva_end = binary_end;
}

void segment_disassembler::generate_blocks()
{
    uint32_t block_start_rva = rva_begin;
    uint32_t current_rva = rva_begin;

    decode_vec block_instructions;
    for (auto& inst: function)
    {
        block_instructions.push_back(inst);

        if (inst.instruction.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE)
        {
            // end of our block
            basic_block* block = new basic_block();
            block->start_rva = block_start_rva;
            block->end_rva_inc = current_rva + inst.instruction.length;
            block->instructions = block_instructions;

            set_block_rvas(block, current_rva);
            set_end_reason(block);

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
        block->end_rva_inc = current_rva + inst.instruction.length;
        block->instructions = block_instructions;

        set_block_rvas(block, current_rva);
        set_end_reason(block);

        blocks.push_back(block);
    }

    // but here we have an issue, some block may be jumping inside of another block
    // we need to fix that
    std::vector<basic_block *> new_blocks;

    for (const basic_block* block: blocks)
    {
        if (block->target_rvas.empty())
            continue;

        const auto& [jump_rva, jump_type] = block->target_rvas.back();
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
                new_block->target_rvas = target_block->target_rvas;

                jump_location location = jump_rva > rva_end || jump_rva < rva_begin ? outside_segment : inside_segment;
                target_block->target_rvas = {{jump_rva, location}};
                target_block->end_rva_inc = jump_rva;
                new_block->end_reason = target_block->end_reason;
                target_block->end_reason = block_end;

                // for the new_block, we copy all the instructions starting at the far_rva
                uint32_t curr_rva = target_block->start_rva;
                for (int i = 0; i < target_block->instructions.size();)
                {
                    zydis_decode& inst = target_block->instructions[i];
                    if (curr_rva >= jump_rva)
                    {
                        // add to new block, remove from old block
                        new_block->instructions.push_back(inst);
                        target_block->instructions.erase(target_block->instructions.begin() + i);
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

    root_block = blocks[0];
    for (basic_block* block: blocks)
    {
        for (auto& [target_rva, rva_type]: block->target_rvas)
        {
            if (rva_type == outside_segment)
                continue;

            for (auto& target_block: blocks)
            {
                if (target_rva == target_block->start_rva)
                {
                    block->target_blocks.push_back(target_block);
                    break;
                }
            }
        }
    }
}

void segment_disassembler::set_end_reason(basic_block* block)
{
    const auto [instruction, operands] = block->instructions.back();
    if (instruction.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE)
    {
        zyids_mnemonic mnemonic = instruction.mnemonic;
        if (mnemonic == ZYDIS_MNEMONIC_JMP)
        {
            block->end_reason = block_jump;
        }
        else
        {
            block->end_reason = block_conditional_jump;
        }
    }
    else
    {
        block->end_reason = block_end;
    }
}

void segment_disassembler::set_block_rvas(basic_block* block, uint32_t current_rva) const
{
    const zydis_decode& inst = block->instructions.back();

    // conditional jumps and regular block ends fall through to the next rva
    if (inst.instruction.mnemonic != ZYDIS_MNEMONIC_JMP)
        block->target_rvas.emplace_back(block->end_rva_inc, get_jump_location(block->end_rva_inc));

    if(inst.instruction.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE)
    {
        // conditional jumps and jumps can either go somewhere else, or next
        uint64_t target_address;
        ZydisCalcAbsoluteAddress(&inst.instruction, &inst.operands[0], current_rva, &target_address);

        block->target_rvas.emplace_back(target_address, get_jump_location(target_address));
    }
}

jump_location segment_disassembler::get_jump_location(const uint32_t rva) const
{
    if(rva >= rva_begin && rva < rva_end)
        return inside_segment;

    return outside_segment;
}
