#pragma once
#include "disassembler/models/basic_block.h"
#include "util/zydis_helper.h"
#include "util/util.h"

class segment_disassembler
{
public:
    explicit segment_disassembler(const decode_vec& segment, uint32_t binary_rva);

    void generate_blocks();

    std::vector<basic_block*> blocks;

private:
    uint32_t rva;
    decode_vec function;

    basic_block* root_block;
};