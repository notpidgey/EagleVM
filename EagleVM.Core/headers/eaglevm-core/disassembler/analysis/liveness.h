#pragma once
#include <set>
#include <unordered_set>
#include <utility>

#include "eaglevm-core/disassembler/dasm.h"
#include "eaglevm-core/disassembler/analysis/models/info.h"

#include "eaglevm-core/util/assert.h"

namespace eagle::dasm::analysis
{
    class liveness
    {
    public:
        std::unordered_map<basic_block_ptr, std::pair<liveness_info, liveness_info>> live;

        explicit liveness(segment_dasm_ptr segment);

        void analyze_cross_liveness(const basic_block_ptr& exit_block);
        std::vector<std::pair<liveness_info, liveness_info>> analyze_block(const basic_block_ptr& block);
        std::pair<liveness_info, liveness_info> analyze_block_at(const basic_block_ptr& block, size_t idx);

        void compute_blocks_use_def();
        static liveness_info compute_inst_flags(const codec::dec::inst_info& inst_info);

    private:
        segment_dasm_ptr segment;
        std::unordered_map<basic_block_ptr, std::pair<liveness_info, liveness_info>> block_use_def;

        static void compute_inst_use_def(codec::dec::inst_info inst_info, liveness_info& use, liveness_info& def);
    };
}
