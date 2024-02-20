#pragma once
#include "disassembler/models/basic_block.h"
#include "util/zydis_helper.h"
#include "util/util.h"

class segment_disassembler
{
public:
    explicit segment_disassembler(instructions_vec& segment, uint32_t rva);

    void generate_blocks();

private:
    uint32_t rva;
    instructions_vec function;

    basic_block* root_block;
};