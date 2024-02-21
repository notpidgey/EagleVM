#pragma once
#include "util/zydis_defs.h"
#include "util/section/code_label.h"

struct basic_block
{
    code_label* start_rva_label = nullptr;

    uint32_t start_rva;
    uint32_t end_rva_inc;

    decode_vec instructions;

    std::vector<uint32_t> target_rvas;
    std::vector<basic_block*> target_blocks;

    bool is_conditional_jump() const
    {
        return target_rvas.size() == 2;
    }

    code_label* get_label()
    {
        if(start_rva_label == nullptr)
            start_rva_label = code_label::create();

        return start_rva_label;
    }
};
