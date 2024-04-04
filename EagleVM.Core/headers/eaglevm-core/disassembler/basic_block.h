#pragma once
#include "eaglevm-core/util/zydis_defs.h"
#include "eaglevm-core/compiler/code_label.h"

#include "models/block_end_reason.h"
#include "models/block_jump_location.h"

namespace eagle::dasm
{
    class basic_block
    {
    public:
        uint64_t start_rva;
        uint64_t end_rva_inc;
        decode_vec decoded_insts;

        basic_block();

        block_end_reason get_end_reason() const;
        uint64_t calc_jump_address(uint8_t instruction_index) const;

        bool is_conditional_jump() const;
        bool is_jump() const;
    };
}

