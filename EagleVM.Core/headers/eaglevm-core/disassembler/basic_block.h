#pragma once
#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/compiler/code_container.h"

#include "models/block_end_reason.h"
#include "models/block_jump_location.h"

namespace eagle::dasm
{
    class basic_block
    {
    public:
        uint64_t start_rva;
        uint64_t end_rva_inc;
        codec::decode_vec decoded_insts;

        basic_block();

        block_end_reason get_end_reason() const;
        uint64_t calc_jump_address(uint32_t index) const;

        bool is_conditional_jump() const;
        bool is_jump() const;

        uint64_t get_index_rva(uint32_t index) const;
    };
}

