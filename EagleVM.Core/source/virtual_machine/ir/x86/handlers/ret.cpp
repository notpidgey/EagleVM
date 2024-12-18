#include "eaglevm-core/virtual_machine/ir/x86/handlers/ret.h"
#include "eaglevm-core/virtual_machine/ir/x86/util.h"
#include "eaglevm-core/virtual_machine/ir/ir_translator.h"

#include "eaglevm-core/virtual_machine/ir/block_builder.h"

#define HS(x) hash_string::hash(x)

namespace eagle::ir
{
    namespace handler
    {
        ret::ret()
        {
            valid_operands = {
                { { { } }, "jcc imm32" },
                { { { } }, "jcc imm16" },
                { { { } }, "jcc imm8" },
            };
        }

        ir_insts ret::gen_handler(const handler_sig signature)
        {
            return base_handler_gen::gen_handler(signature);
        }
    }

#include "eaglevm-core/virtual_machine/ir/ir_translator.h"

    namespace lifter
    {
        bool ret::translate_to_il(const uint64_t original_rva, const x86_cpu_flag flags)
        {
            // not sure what to do about this, feels that it should be a vmexit
            block->push_back(std::make_shared<cmd_jmp>());
            return true;
        }
    }
}
