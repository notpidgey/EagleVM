#pragma once
#include "util/zydis_defs.h"

struct basic_block
{
    uint32_t start_rva;
    uint32_t end_rva;

    dynamic_instructions_vec instructions;
    std::vector<basic_block*> target_blocks;
};