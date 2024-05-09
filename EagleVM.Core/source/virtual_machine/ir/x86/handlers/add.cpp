#include "eaglevm-core/virtual_machine/ir/x86/handlers/add.h"

#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_rflags_store.h"

namespace eagle::il::handler
{
    add::add()
    {
        entries = {
            { { codec::op_none, codec::bit_8 }, { codec::op_none, codec::bit_8 } },
            { { codec::op_none, codec::bit_16 }, { codec::op_none, codec::bit_16 } },
            { { codec::op_none, codec::bit_32 }, { codec::op_none, codec::bit_32 } },
            { { codec::op_none, codec::bit_64 }, { codec::op_none, codec::bit_64 } },
        };
    }

    il_insts add::gen_handler(const codec::reg_class size, uint8_t operands)
    {
        // the way this is done is far slower than it used to be
        // however because of the way this IL is written, there is far more room to expand how the virtual context is stored
        // in addition, it gives room for mapping x86 context into random places as well

        const il_size target_size = static_cast<il_size>(get_reg_size(size));
        const reg_vm vtemp = get_bit_version(reg_vm::vtemp, target_size);
        const reg_vm vtemp2 = get_bit_version(reg_vm::vtemp2, target_size);

        // TODO: this needs to be marked as rflags sensitive
        return {
            // there should be some kind of indication that we dont care what temp registers we pop into
            // create some kind of parameter holder which will tell the x86 code gen that we will assign this register to this random temp and this to other temp
            // todo: to this ^ maybe? or have it automatically happen by some obfuscation pass
            std::make_shared<cmd_vm_pop>(vtemp, target_size),
            std::make_shared<cmd_vm_pop>(vtemp2, target_size),
            std::make_shared<cmd_x86_dynamic>(codec::m_add, vtemp, vtemp2),
            std::make_shared<cmd_vm_push>(vtemp, target_size)
        };
    }
}

namespace eagle::il::lifter
{
    void add::finalize_translate_to_virtual()
    {
        block->add_command(std::make_shared<cmd_rflags_load>());
        base_x86_lifter::finalize_translate_to_virtual();
        block->add_command(std::make_shared<cmd_rflags_store>());
    }
}
