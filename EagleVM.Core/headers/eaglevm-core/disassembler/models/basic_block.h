#pragma once
#include "eaglevm-core/util/zydis_defs.h"
#include "eaglevm-core/util/section/code_label.h"

#include "eaglevm-core/disassembler/models/block_end_reason.h"
#include "eaglevm-core/disassembler/models/block_jump_location.h"

class basic_block
{
public:
    uint32_t start_rva;
    uint32_t end_rva_inc;
    decode_vec decoded_insts;

    basic_block();

    block_end_reason get_end_reason() const;
    uint64_t calc_jump_address(uint8_t instruction_index) const;

    bool is_conditional_jump() const;
    bool is_jump() const;
};
