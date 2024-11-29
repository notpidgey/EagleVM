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
        uint64_t target_rva;
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
        virtual uint64_t get_current_rva() = 0;

        /// @brief updates the current rva
        /// @param rva new rva
        /// @return old rva before replacement
        virtual uint64_t set_current_rva(uint64_t rva) = 0;
    };

    class base_segment_dasm : public segment_dasm_kernel
    {
    public:
        virtual std::vector<basic_block_ptr> explore_blocks(uint64_t entry_rva) = 0;

        /// @brief gets all the instructions in a certain section and disregards the flow of the instructions
        /// @param rva_begin the rva at which the starting instruction is at
        /// @param rva_end the inclusive rva at which the last instruction ends
        /// @return the list of instructions which are contained within this range
        virtual basic_block_ptr dump_section(uint64_t rva_begin, uint64_t rva_end) = 0;

        /// @brief dissasembled instructions until a branching instruction is reached at the current block
        /// @param rva the rva at which the target block begins
        /// @return the basic block the instructions create
        virtual basic_block_ptr get_block(uint32_t rva) = 0;
    };

    class segment_dasm : public base_segment_dasm
    {
    public:
        segment_dasm(uint64_t rva_base, uint8_t* buffer, size_t size);

        std::vector<basic_block_ptr> explore_blocks(uint64_t entry_rva) override;
        basic_block_ptr get_block(uint32_t rva) override;

        std::vector<basic_block_ptr>& get_blocks();

    private:
        uint64_t rva_base;
        uint8_t* instruction_buffer;
        size_t instruction_size;
        
        std::vector<basic_block_ptr> blocks;

        std::pair<codec::dec::inst_info, uint8_t> decode_current() override;
        std::vector<branch_info_t> get_branches() override;
    };

    using segment_dasm_ptr = std::shared_ptr<segment_dasm>;
}
