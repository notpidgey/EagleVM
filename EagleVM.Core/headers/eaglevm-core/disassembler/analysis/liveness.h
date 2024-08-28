#pragma once
#include <set>
#include <unordered_set>
#include <utility>
#include "eaglevm-core/disassembler/disassembler.h"

namespace eagle::dasm::analysis
{
    using reg_set = std::set<codec::reg>;
    class liveness
    {
    public:
        std::unordered_map<basic_block_ptr, reg_set> in;
        std::unordered_map<basic_block_ptr, reg_set> out;

        explicit liveness(segment_dasm_ptr segment);

        void analyze_cross_liveness(const basic_block_ptr& exit_block);
        std::vector<std::pair<reg_set, reg_set>> analyze_block_liveness(const basic_block_ptr& block);

        void compute_use_def();

    private:
        segment_dasm_ptr segment;
        std::unordered_map<basic_block_ptr, std::pair<reg_set, reg_set>> block_use_def;

        static void compute_use_def(codec::dec::inst_info inst_info, reg_set& use, reg_set& def);
    };
}
