#include "eaglevm-core/virtual_machine/ir/x86/handlers/dec.h"

#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_store.h"

namespace eagle::ir::handler
{
    dec::dec()
    {
        // todo: make vector of supported signatures
        // todo: make vector of handlers to generate
        valid_operands = {
            operand_signature{ handler_op{ codec::op_none, codec::bit_8 }, "8" },
            operand_signature{ handler_op{ codec::op_none, codec::bit_16 }, "16" },
            operand_signature{ handler_op{ codec::op_none, codec::bit_32 }, "32" },
            operand_signature{ handler_op{ codec::op_none, codec::bit_64 }, "64" },
        };
    }

    ir_insts dec::gen_handler(codec::reg_class size, uint8_t operands)
    {
        const ir_size target_size = static_cast<ir_size>(get_reg_size(size));
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
    void dec::finalize_translate_to_virtual()
    {
        block->add_command(std::make_shared<cmd_rflags_load>());
        base_x86_translator::finalize_translate_to_virtual();
        block->add_command(std::make_shared<cmd_rflags_store>());
    }
}
