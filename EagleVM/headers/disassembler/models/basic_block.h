#pragma once
#include "util/zydis_defs.h"
#include "util/section/code_label.h"

enum block_end_reason
{
    block_end,
    block_jump,
    block_conditional_jump,
};

struct basic_block
{
    block_end_reason end_reason = block_end;

    uint32_t start_rva;
    uint32_t end_rva_inc;

    decode_vec instructions;

    std::vector<uint32_t> target_rvas;
    std::vector<basic_block*> target_blocks;

    bool is_conditional_jump() const
    {
        const auto& [instruction, _] = instructions.back();
        return instruction.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE && instruction.mnemonic != ZYDIS_MNEMONIC_JMP;
    }

    bool is_final_block() const
    {
        return target_rvas.empty();
    }
};
