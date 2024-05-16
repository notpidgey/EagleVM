#include "eaglevm-core/virtual_machine/ir/x86/handlers/inc.h"

#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_store.h"

namespace eagle::ir::handler
{
    inc::inc()
    {
        entries = {
            { { codec::op_none, codec::bit_8 } },
            { { codec::op_none, codec::bit_16 } },
            { { codec::op_none, codec::bit_32 } },
            { { codec::op_none, codec::bit_64 } },
        };
    }

    ir_insts inc::gen_handler(const codec::reg_class size, uint8_t operands)
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
    void inc::finalize_translate_to_virtual()
    {
        block->add_command(std::make_shared<cmd_rflags_load>());
        base_x86_translator::finalize_translate_to_virtual();
        block->add_command(std::make_shared<cmd_rflags_store>());
    }
}
