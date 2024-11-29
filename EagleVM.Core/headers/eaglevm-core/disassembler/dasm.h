#pragma once
#include <queue>
#include <tuple>

#include "eaglevm-core/disassembler/models/branch_info.h"
#include "eaglevm-core/disassembler/basic_block.h"
#include "eaglevm-core/codec/zydis_helper.h"

namespace eagle::dasm
{
    class segment_dasm_kernel
    {
    public:
        virtual ~segment_dasm_kernel() = default;

    protected:
        /// @brief decodes an instruction at the specified rva.
        virtual std::pair<codec::dec::inst_info, uint8_t> decode_instruction(uint64_t rva) = 0;

        /// @brief retrieves branches for the instruction at the specified rva.
        virtual std::vector<branch_info_t> get_branches(uint64_t rva) = 0;
    };

    class base_segment_dasm : public segment_dasm_kernel
    {
    public:
        virtual std::vector<basic_block_ptr> explore_blocks(uint64_t entry_rva) = 0;

        /// @brief gets all the instructions in a certain section and disregards the flow of the instructions
        /// @param rva_begin the rva at which the starting instruction is at
        /// @param rva_end the inclusive rva at which the last instruction ends
        /// @return the list of instructions which are contained within this range
        // virtual basic_block_ptr dump_section(uint64_t rva_begin, uint64_t rva_end) = 0;

        /// @brief dissasembled instructions until a branching instruction is reached at the current block
        /// @param rva the rva at which the target block begins
        /// @return the basic block the instructions create
        virtual basic_block_ptr get_block(uint32_t rva, bool inclusive) = 0;
    };

    class segment_dasm : public base_segment_dasm
    {
    public:
        segment_dasm(uint64_t rva_base, uint8_t* buffer, size_t size);

        std::vector<basic_block_ptr> explore_blocks(uint64_t entry_rva) override;
        basic_block_ptr get_block(uint32_t rva, bool inclusive = false) override;

        std::vector<basic_block_ptr>& get_blocks();

    private:
        uint64_t rva_base;
        uint8_t* instruction_buffer;
        size_t instruction_size;
        
        std::vector<basic_block_ptr> blocks;

        std::pair<codec::dec::inst_info, uint8_t> decode_instruction(uint64_t rva) override;
        std::vector<branch_info_t> get_branches(uint64_t rva) override;
    };

    using segment_dasm_ptr = std::shared_ptr<segment_dasm>;
}
