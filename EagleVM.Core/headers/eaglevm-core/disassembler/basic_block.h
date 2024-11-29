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

        [[nodiscard]] block_end_reason get_end_reason() const;
        [[nodiscard]] uint64_t calc_jump_address(uint32_t index) const;

        [[nodiscard]] bool is_conditional_jump() const;
        [[nodiscard]] bool is_jump() const;

        [[nodiscard]] uint64_t get_index_rva(uint32_t index) const;
    };

    using basic_block_ptr = std::shared_ptr<basic_block>;
}

