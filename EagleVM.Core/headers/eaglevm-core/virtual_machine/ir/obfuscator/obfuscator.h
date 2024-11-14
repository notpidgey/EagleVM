#pragma once
#include "eaglevm-core/disassembler/analysis/liveness.h"
#include "eaglevm-core/virtual_machine/ir/ir_translator.h"

namespace eagle::ir
{
    class obfuscator
    {
    public:
        static void run_preopt_pass(
            const preopt_block_vec& preopt_vec,
            const dasm::analysis::liveness* liveness
        );

        /**
         * combines list of ir instructions into a single block
         * existing sequence of instructions will be replaced across all blocks into a single handler call
         * @param blocks ir blocks containing
         */
        static std::vector<block_ptr> create_merged_handlers(
            const std::vector<block_ptr>& blocks
        );
    };
}
