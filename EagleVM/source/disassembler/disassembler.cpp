#include "disassembler/disassembler.h"

segment_disassembler::segment_disassembler(const decode_vec &segment, const uint32_t binary_rva) :
    root_block(nullptr)
{
    function = segment;
    rva = binary_rva;
}

void segment_disassembler::generate_blocks()
{
    uint32_t block_start_rva = rva;
    uint32_t current_rva = rva;

    decode_vec block_instructions;
    for (int i = 0; i < function.size(); i++)
    {
        zydis_decode &inst = function[i];
        block_instructions.push_back(inst);

        const zyids_mnemonic mnemonic = inst.instruction.mnemonic;
        if (mnemonic >= ZYDIS_MNEMONIC_JB && mnemonic <= ZYDIS_MNEMONIC_JZ)
        {
            // end of our block
            basic_block *block = new basic_block();
            block->start_rva = block_start_rva;
            block->end_rva_inc = current_rva + inst.instruction.length;
            block->instructions = block_instructions;

            const zydis_decoded_operand op = inst.operands[0];
            const auto immediate = op.imm;

            if (mnemonic != ZYDIS_MNEMONIC_JMP)
                block->target_rvas.push_back(block->end_rva_inc);

            // conditional jumps can either go somewhere else, or next
            uint64_t target_address;
            ZydisCalcAbsoluteAddress(&inst.instruction, &op, current_rva, &target_address);

            block->target_rvas.push_back(target_address);

            blocks.push_back(block);
            block_instructions.clear();

            block_start_rva = block->end_rva_inc;
        }

        current_rva += inst.instruction.length;
    }

    if (!block_instructions.empty())
    {
        zydis_decode &inst = function.back();

        basic_block *block = new basic_block();
        block->start_rva = block_start_rva;
        block->end_rva_inc = current_rva + inst.instruction.length;
        block->instructions = block_instructions;

        blocks.push_back(block);
    }

    // but here we have an issue, some block may be jumping inside of another block
    // we need to fix that
    std::vector<basic_block *> new_blocks;

    for (auto &block : blocks)
    {
        if (block->target_rvas.empty())
            continue;

        const uint32_t jump_rva = block->target_rvas.back();
        for (const auto &target_block : blocks)
        {
            // non inclusive is key because we might already be at that block
            if (jump_rva > target_block->start_rva && jump_rva < target_block->end_rva_inc)
            {
                // we found a jump to the middle of a block
                // we need to split the block

                basic_block *new_block = new basic_block();
                new_block->start_rva = jump_rva;
                new_block->end_rva_inc = target_block->end_rva_inc;
                target_block->end_rva_inc = jump_rva;
                new_block->target_rvas = target_block->target_rvas;

                target_block->target_rvas = {jump_rva};
                target_block->end_rva_inc = jump_rva;

                // for the new_block, we copy all the instructions starting at the far_rva
                uint32_t curr_rva = target_block->start_rva;
                for (int i = 0; i < target_block->instructions.size();)
                {
                    zydis_decode &inst = target_block->instructions[i];
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

                    curr_rva += inst.instruction.length;
                }

                new_blocks.push_back(new_block);
            }
        }
    }

    blocks.insert(blocks.end(), new_blocks.begin(), new_blocks.end());

    root_block = blocks[0];
    for (auto &block : blocks)
    {
        // For each block, iterate over all its target_rvas (middle loop)
        for (auto &target_rva : block->target_rvas)
        {
            // For each target_rva, iterate over all blocks again (inner loop)
            for (auto &target_block : blocks)
            {
                // If the target_rva matches the start_rva of any block
                if (target_rva == target_block->start_rva)
                {
                    // Add that block to the target_blocks of the current block
                    block->target_blocks.push_back(target_block);
                }
            }
        }
    }
}
