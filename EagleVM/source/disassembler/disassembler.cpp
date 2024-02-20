#include "disassembler/disassembler.h"

segment_disassembler::segment_disassembler(const decode_vec& segment, const uint32_t binary_rva)
    : root_block(nullptr)
{
    function = segment;
    rva = binary_rva;
}

void segment_disassembler::generate_blocks()
{
    uint32_t block_start_rva = rva;
    uint32_t current_rva = rva;

    decode_vec block_instructions;
    for(int i = 0; i < function.size(); i++)
    {
        zydis_decode& inst = function[i];
        block_instructions.push_back(inst);

        const zyids_mnemonic mnemonic = inst.instruction.mnemonic;
        if(mnemonic >= ZYDIS_MNEMONIC_JB && mnemonic <= ZYDIS_MNEMONIC_JZ)
        {
            // end of our block
            basic_block* block = new basic_block();
            block->start_rva = block_start_rva;
            block->end_rva = current_rva;
            block->end_rva_inc = current_rva + inst.instruction.length;
            block->instructions = block_instructions;

            const zydis_decoded_operand op = inst.operands[0];
            const auto immediate = op.imm;

            // conditional jumps can either go somewhere else, or next
            block->target_rvas.push_back(current_rva + immediate.value.u + 2);
            block->target_rvas.push_back(block->end_rva_inc);

            blocks.push_back(block);
            block_instructions.clear();

            block_start_rva = block->end_rva_inc;
        }

        current_rva += inst.instruction.length;
    }

    if(!block_instructions.empty())
    {
        zydis_decode& inst = function.back();

        basic_block* block = new basic_block();
        block->start_rva = block_start_rva;
        block->end_rva = current_rva;
        block->end_rva_inc = current_rva + inst.instruction.length;
        block->instructions = block_instructions;

        const zydis_decoded_operand op = inst.operands[0];
        const auto immediate = op.imm;

        block->target_rvas.push_back(current_rva + immediate.value.u + 2);
        block->target_rvas.push_back(block->end_rva_inc);

        blocks.push_back(block);
    }

    // but here we have an issue, some block may be jumping inside of another block
    // we need to fix that
    std::vector<basic_block*> new_blocks;

    for(auto& block : blocks)
    {
        const uint32_t far_rva = block->target_rvas.back();
        for(const auto& target_block : blocks)
        {
            if(far_rva > target_block->start_rva && far_rva < target_block->end_rva_inc)
            {
                // we found a jump to the middle of a block
                // we need to split the block
                basic_block* new_block = new basic_block();
                new_block->start_rva = far_rva;
                new_block->end_rva = target_block->end_rva;
                new_block->end_rva_inc = target_block->end_rva_inc;

                // for the new_block, we copy all the instructions starting at the far_rva
                uint32_t current_rva = target_block->start_rva;
                for(int i = 0; i < target_block->instructions.size(); i++)
                {
                    zydis_decode& inst = target_block->instructions[i];
                    if(current_rva >= far_rva)
                    {
                        new_block->instructions.push_back(inst);

                        // also remove the instruction from the old block
                        target_block->instructions.erase(target_block->instructions.begin() + i);
                    }

                    current_rva += inst.instruction.length;
                }

                target_block->end_rva_inc = far_rva;
                target_block->end_rva = far_rva;

                new_blocks.push_back(new_block);
            }
        }
    }

    blocks.insert(blocks.end(), new_blocks.begin(), new_blocks.end());

    root_block = blocks[0];
    for(auto& block : blocks)
    {
        // For each block, iterate over all its target_rvas (middle loop)
        for(auto& target_rva : block->target_rvas)
        {
            // For each target_rva, iterate over all blocks again (inner loop)
            for(auto& target_block : blocks)
            {
                // If the target_rva matches the start_rva of any block
                if(target_rva == target_block->start_rva)
                {
                    // Add that block to the target_blocks of the current block
                    block->target_blocks.push_back(target_block);
                }
            }
        }
    }
}