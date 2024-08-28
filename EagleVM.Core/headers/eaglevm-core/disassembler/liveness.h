#pragma once
#include <set>
#include <unordered_set>
#include <utility>
#include "eaglevm-core/disassembler/disassembler.h"

namespace eagle::dasm
{
    using reg_set = std::set<codec::reg>;
    class liveness_anal
    {
    public:
        std::unordered_map<basic_block_ptr, reg_set> in;
        std::unordered_map<basic_block_ptr, reg_set> out;

        explicit liveness_anal(segment_dasm_ptr segment);

        void analyze(const basic_block_ptr& exit_block);
        void compute_use_def();

    private:
        segment_dasm_ptr segment;
        std::unordered_map<basic_block_ptr, std::pair<reg_set, reg_set>> block_use_def;
    };
}
