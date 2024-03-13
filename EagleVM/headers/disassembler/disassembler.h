#pragma once
#include "disassembler/models/basic_block.h"
#include "util/zydis_helper.h"
#include "util/util.h"

class segment_dasm
{
public:
    explicit segment_dasm(const decode_vec& segment, uint32_t binary_rva, uint32_t binary_end);

    void generate_blocks();

    std::pair<uint64_t, block_jump_location> get_jump(const basic_block* block, bool last = false);
    block_jump_location get_jump_location(uint32_t rva) const;

    basic_block* get_block(uint32_t rva) const;

    std::vector<basic_block*> blocks;
    basic_block* root_block;

private:
    uint32_t rva_begin;
    uint32_t rva_end;
    decode_vec function;
};