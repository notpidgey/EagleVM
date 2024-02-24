#pragma once
#include "util/zydis_defs.h"
#include "util/section/code_label.h"

#include "disassembler/models/block_end_reason.h"
#include "disassembler/models/block_jump_location.h"

class basic_block
{
public:
    uint32_t start_rva;
    uint32_t end_rva_inc;

    block_end_reason end_reason = block_end;

    decode_vec instructions;
    std::vector<std::pair<uint32_t, block_jump_location>> target_rvas;
    std::vector<basic_block*> target_blocks;

    basic_block();

    bool is_conditional_jump() const;
    bool is_jump() const;
    bool is_final_block() const;
};
