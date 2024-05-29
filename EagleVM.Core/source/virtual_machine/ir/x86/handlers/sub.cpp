#include "eaglevm-core/virtual_machine/ir/x86/handlers/sub.h"

#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_store.h"

namespace eagle::ir::handler
{
    sub::sub()
    {
        valid_operands = {
            { { { codec::op_none, codec::bit_8 }, { codec::op_none, codec::bit_8 } }, "sub 8,8" },
            { { { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_16 } }, "sub 16,16" },
            { { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_32 } }, "sub 32,32" },
            { { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_64 } }, "sub 64,64" },
        };

        build_options = {
            { { ir_size::bit_8, ir_size::bit_8 }, "sub 8,8" },
            { { ir_size::bit_16, ir_size::bit_16 }, "sub 16,16" },
            { { ir_size::bit_32, ir_size::bit_32 }, "sub 32,32" },
            { { ir_size::bit_64, ir_size::bit_64 }, "sub 64,64" },
        };
    }

    ir_insts sub::gen_handler(const handler_sig signature)
    {
        assert(signature.size() == 2, "invalid signature. must contain 2 operands");
        assert(signature[0] == signature[1], "invalid signature. must contain same sized parameters");

        ir_size target_size = signature.front();

        const discrete_store_ptr vtemp = discrete_store::create(target_size);
        const discrete_store_ptr vtemp2 = discrete_store::create(target_size);

        // todo: some kind of virtual machine implementation where it could potentially try to optimize a pop and use of the register in the next
        // instruction using stack dereference
        return {
            std::make_shared<cmd_pop>(vtemp, target_size),
            std::make_shared<cmd_pop>(vtemp2, target_size)->block_write(vtemp),
            std::make_shared<cmd_x86_dynamic>(codec::m_sub, vtemp, vtemp2),
            std::make_shared<cmd_push>(vtemp, target_size)
        };
    }
}

namespace eagle::ir::lifter
{
    void sub::finalize_translate_to_virtual()
    {
        block->add_command(std::make_shared<cmd_rflags_load>());
        base_x86_translator::finalize_translate_to_virtual();
        block->add_command(std::make_shared<cmd_rflags_store>());

        codec::reg target_reg = static_cast<codec::reg>(operands[0].reg.value);
        block->add_command(std::make_shared<cmd_context_store>(target_reg));
    }
}