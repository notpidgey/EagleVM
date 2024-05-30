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
            { { { codec::op_none, codec::bit_8 } }, "dec 8" },
            { { { codec::op_none, codec::bit_16 } }, "dec 16" },
            { { { codec::op_none, codec::bit_32 } }, "dec 32" },
            { { { codec::op_none, codec::bit_64 } }, "dec 64" },
        };

        build_options = {
            { { ir_size::bit_8 }, "dec 8" },
            { { ir_size::bit_16 }, "dec 16" },
            { { ir_size::bit_32 }, "dec 32" },
            { { ir_size::bit_64 }, "dec 64" },
        };
    }

    ir_insts dec::gen_handler(handler_sig signature)
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
    bool dec::virtualize_as_address(codec::dec::operand operand, uint8_t idx)
    {
        return base_x86_translator::virtualize_as_address(operand, idx);
    }

    void dec::finalize_translate_to_virtual()
    {
        block->add_command(std::make_shared<cmd_rflags_load>());
        base_x86_translator::finalize_translate_to_virtual();
        block->add_command(std::make_shared<cmd_rflags_store>());

        codec::dec::operand first_op = operands[0];
        if(first_op.type == ZYDIS_OPERAND_TYPE_REGISTER)
        {
            // register
            codec::reg reg = static_cast<codec::reg>(first_op.reg.value);
            block->add_command(std::make_shared<cmd_context_store>(reg));
        }
        else
        {
            // memory
        }

    }
}
