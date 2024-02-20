#pragma once
#include "util/zydis_defs.h"

struct basic_block
{
    uint32_t start_rva;
    uint32_t end_rva;
    uint32_t end_rva_inc;

    decode_vec instructions;

    std::vector<uint32_t> target_rvas;
    std::vector<basic_block*> target_blocks;
};