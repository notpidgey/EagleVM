#include "eaglevm-core/virtual_machine/ir/x86/handlers/inc.h"

#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_store.h"

namespace eagle::ir::handler
{
    inc::inc()
    {
        valid_operands = {
            { { { codec::op_none, codec::bit_8 } }, "inc 8" },
            { { { codec::op_none, codec::bit_16 } }, "inc 16" },
            { { { codec::op_none, codec::bit_32 } }, "inc 32" },
            { { { codec::op_none, codec::bit_64 } }, "inc 64" },
        };

        build_options = {
            { { ir_size::bit_8 }, "inc 8" },
            { { ir_size::bit_16 }, "inc 16" },
            { { ir_size::bit_32 }, "inc 32" },
            { { ir_size::bit_64 }, "inc 64" },
        };
    }

    ir_insts inc::gen_handler(handler_sig signature)
    {
        assert(signature.size() == 1, "invalid signature. must contain 1 operand");
        ir_size target_size = signature.front();

        const discrete_store_ptr vtemp = discrete_store::create(target_size);

        return {
            std::make_shared<cmd_pop>(vtemp, target_size),
            std::make_shared<cmd_x86_dynamic>(codec::m_dec, vtemp),
            std::make_shared<cmd_push>(vtemp, target_size)
        };
    }
}

namespace eagle::ir::lifter
{
    void inc::finalize_translate_to_virtual()
    {
        block->add_command(std::make_shared<cmd_rflags_load>());
        base_x86_translator::finalize_translate_to_virtual();
        block->add_command(std::make_shared<cmd_rflags_store>());
    }
}
