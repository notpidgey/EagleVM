#pragma once
#include <cstdint>
#include <queue>
#include <tuple>

#include "eaglevm-core/disassembler/basic_block.h"
#include "eaglevm-core/codec/zydis_helper.h"

namespace eagle::dasm
{
    struct branch_info_t
    {
        // if false, this means that the branch could not be resolved
        // this means that the branch is stored in a register of some sort
        bool is_resolved;

        // valid branching control flow which cannot be resolved
        bool is_ret;

        // target rva of branch
        uint32_t target_rva;
    };

    class segment_dasm_kernel
    {
    public:
        virtual ~segment_dasm_kernel() = default;

    protected:
        /// @brief decodes an instruction at the current rva, the function assumes that the rva is located at a valid instruction
        /// @return pair with [decoded instruction, instruction length] at the current rva
        virtual std::pair<codec::dec::inst_info, uint8_t> decode_current() = 0;

        /// @brief decodes the instruction at the current rva and returns branches
        /// @return returns a list of rvas the instruction branches to. len(0) if none, len(1) if jmp, len(2) if conditional jump
        virtual std::vector<branch_info_t> get_branches() = 0;

        /// @brief getter for the current rva
        /// @return the current rva
        virtual uint32_t get_current_rva() = 0;

        /// @brief updates the current rva
        /// @param rva new rva
        /// @return old rva before replacement
        virtual uint32_t set_current_rva(uint32_t rva) = 0;
    };

    class base_segment_dasm : public segment_dasm_kernel
    {
    public:
        virtual std::vector<basic_block> explore_blocks(uint32_t entry_rva) = 0;

    private:
        /// @brief dissasembled instructions until a branching instruction is reached at the current block
        /// @param rva the rva at which the target block begins
        /// @return the basic block the instructions create
        virtual basic_block get_block(uint32_t rva) = 0;

        /// @brief gets all the instructions in a certain section and disregards the flow of the instructions
        /// @param rva_begin the rva at which the starting instruction is at
        /// @param rva_end the inclusive rva at which the last instruction ends
        /// @return the list of instructions which are contained within this range
        virtual std::vector<codec::dec::inst> dump_section(uint32_t rva_begin, uint32_t rva_end) = 0;
    };

    class segment_dasm : public base_segment_dasm
    {
    public:
        segment_dasm(uint8_t* buffer, size_t size)
            : instruction_buffer(buffer), instruction_size(size)
        {

        }

        std::vector<basic_block> explore_blocks(const uint32_t entry_rva) override
        {
            std::vector<basic_block> collected_blocks;

            std::queue<uint32_t> explore_queue;
            explore_queue.push(entry_rva);

            while (!explore_queue.empty())
            {
                const size_t layer_size = explore_queue.size();
                for (int i = 0; i < layer_size; i++)
                {
                    const uint32_t layer_rva = explore_queue.front();
                    explore_queue.pop();

                    set_current_rva(layer_rva);

                    // check if we are in the middle of an already existing block
                    // if so, we split up the block and we just continue
                    for (auto& created_block : collected_blocks)
                    {
                        if (layer_rva >= created_block.start_rva && layer_rva < created_block.end_rva_inc)
                        {
                            basic_block previous_block;
                            previous_block.start_rva = created_block.start_rva;
                            previous_block.end_rva_inc = previous_block.start_rva;

                            created_block.start_rva = layer_rva;

                            while (previous_block.end_rva_inc < layer_rva)
                            {
                                auto block_inst = created_block.decoded_insts.front();
                                previous_block.decoded_insts.push_back(block_inst);
                                created_block.decoded_insts.erase(created_block.decoded_insts.begin());

                                previous_block.end_rva_inc += block_inst.instruction.length;
                            }

                            // this means there is some tricky control flow happening
                            // for instance, there may be an opaque branch to some garbage code
                            // another reason could be is we explored the wrong branch first of some obfuscated code and found garbage
                            // this will not happen with normally compiled code
                            VM_ASSERT(previous_block.end_rva_inc == layer_rva, "resulting jump is between an already explored instruction");
                            collected_blocks.push_back(previous_block);
                        }
                    }

                    basic_block block = { };
                    block.start_rva = get_current_rva();

                    while (true)
                    {
                        // we must do a check to see if our rva is at some already existing block,
                        // if so, we are going to end this block
                        const auto current_rva = get_current_rva();
                        for (const auto& created_block : collected_blocks)
                        {
                            if (current_rva >= created_block.start_rva && current_rva < created_block.end_rva_inc)
                            {
                                // we are inside
                                VM_ASSERT(current_rva == created_block.start_rva, "instruction overlap caused by seeking block");
                                break;
                            }
                        }

                        auto [decode_info, inst_size] = decode_current();
                        block.decoded_insts.push_back(decode_info);

                        set_current_rva(get_current_rva() + inst_size);

                        if (decode_info.instruction.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE)
                        {
                            // branching instruction encountered
                            std::vector<branch_info_t> branches = get_branches();
                            for (auto& [is_resolved, is_ret, target_rva] : branches)
                            {
                                if (is_resolved)
                                    explore_queue.push(target_rva);
                            }

                            break;
                        }
                    }

                    block.end_rva_inc = get_current_rva();
                    collected_blocks.push_back(block);
                }
            }

            return collected_blocks;
        }

    private:
        uint8_t* instruction_buffer;
        size_t instruction_size;

        std::vector<branch_info_t> get_branches() override
        {
            auto inst = codec::get_instruction(instruction_buffer, instruction_size);

        }
    };
}
