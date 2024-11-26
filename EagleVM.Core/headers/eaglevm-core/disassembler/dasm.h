#pragma once
#include <cstdint>
#include <tuple>

#include "eaglevm-core/disassembler/basic_block.h"
#include "eaglevm-core/codec/zydis_helper.h"

namespace eagle::dasm
{
    class segment_dasm_kernel
    {
    protected:
        /// @brief decodes an instruction at the current rva, the function assumes that the rva is located at a valid instruction
        /// @return pair with [decoded instruction, instruction length] at the current rva
        virtual std::pair<codec::dec::inst, uint8_t> decode_current() = 0;

        /// @brief decodes the instruction at the current rva and returns branches
        /// @return returns a list of rvas the instruction branches to. len(0) if none, len(1) if jmp, len(2) if conditional jump
        virtual std::vector<uint32_t> get_branches() = 0;

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
        virtual std::vector<basic_block> explore_blocks(uint32_t rva) = 0;

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

    };
}
