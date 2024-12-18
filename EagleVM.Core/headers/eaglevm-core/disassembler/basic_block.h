#pragma once
#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/compiler/code_container.h"

#include "eaglevm-core/disassembler/models/block_end_reason.h"
#include "eaglevm-core/disassembler/models/block_jump_location.h"
#include "eaglevm-core/disassembler/models/branch_info.h"

namespace eagle::dasm
{
    struct basic_block
    {
        uint64_t start_rva;
        uint64_t end_rva_inc;

        codec::decode_vec decoded_insts;
        std::vector<branch_info_t> branches;

        basic_block();

        [[nodiscard]] block_end_reason get_end_reason() const;
        [[nodiscard]] bool is_conditional_jump() const;
        [[nodiscard]] bool is_jump() const;

        [[nodiscard]] uint64_t get_index_rva(uint32_t index) const;
        uint64_t get_block_size();
    };

    using basic_block_ptr = std::shared_ptr<basic_block>;
}

