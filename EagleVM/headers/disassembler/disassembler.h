#pragma once
#include "disassembler/models/basic_block.h"
#include "util/zydis_helper.h"
#include "util/util.h"

class segment_disassembler
{
public:
    explicit segment_disassembler(const decode_vec& segment, uint32_t binary_rva, uint32_t binary_end);

    void generate_blocks();
    void set_end_reason(basic_block* block);
    void set_block_rvas(basic_block* block, uint32_t current_rva) const;
    block_jump_location get_jump_location(uint32_t rva) const;


    std::vector<basic_block*> blocks;
    basic_block* root_block;

private:
    uint32_t rva_begin;
    uint32_t rva_end;
    decode_vec function;
};