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
            const std::unique_ptr<dasm::analysis::liveness>& liveness
        );

    private:
    };
}
