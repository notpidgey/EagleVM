#include "eaglevm-core/virtual_machine/ir/x86/handlers/lea.h"

namespace eagle::ir::handler
{
    lea::lea()
    {
        entries = {
            { { codec::op_reg, codec::bit_16 }, { codec::op_mem, codec::bit_32 } },
            { { codec::op_reg, codec::bit_16 }, { codec::op_mem, codec::bit_64 } },

            { { codec::op_reg, codec::bit_32 }, { codec::op_mem, codec::bit_32 } },
            { { codec::op_reg, codec::bit_32 }, { codec::op_mem, codec::bit_64 } },

            { { codec::op_reg, codec::bit_64 }, { codec::op_mem, codec::bit_32 } },
            { { codec::op_reg, codec::bit_64 }, { codec::op_mem, codec::bit_64 } },
        };
    }

    ir_insts lea::gen_handler(codec::reg_class size, uint8_t operands)
    {
        const il_size target_size = static_cast<il_size>(get_reg_size(size));
        const reg_vm vtemp = get_bit_version(reg_vm::vtemp, target_size);
        const reg_vm vtemp2 = get_bit_version(reg_vm::vtemp2, target_size);

        return {
            std::make_shared<cmd_pop>(vtemp, target_size),
            // contains memory address
            std::make_shared<cmd_pop>(vtemp2, target_size),
            // contains destination
            std::make_shared<cmd_context_store>(vtemp2, vtemp, target_size),
            // [vtemp2]
            std::make_shared<cmd_push>(vtemp, target_size)
        };
    }
}

namespace eagle::ir::lifter
{
    bool lea::virtualize_as_address(codec::dec::operand operand, uint8_t idx)
    {
        // this will only hit for the second operand and we will say yes
        return true;
    }

    void lea::finalize_translate_to_virtual()
    {
        // because of our LEA implementation we do not want to call a handler
        // a handler would serve no purpose the way this IL is setup

        // at this point we have the [] memory operand as an address on the stack
        // all we need to do is write it to the register

        codec::zydis_register reg = operands[0].reg.value;
        codec::dec::op_mem mem = operands[1].mem;

        /*
        16 	32 	32-bit effective address is calculated (using 67H prefix). The lower 16 bits of the address are stored in the requested 16-bit register destination (using 66H prefix).
        16 	64 	64-bit effective address is calculated (default address size). The lower 16 bits of the address are stored in the requested 16-bit register destination (using 66H prefix).
        32 	32 	32-bit effective address is calculated (using 67H prefix) and stored in the requested 32-bit register destination.
        32 	64 	64-bit effective address is calculated (default address size) and the lower 32 bits of the address are stored in the requested 32-bit register destination.
        64 	32 	32-bit effective address is calculated (using 67H prefix), zero-extended to 64-bits, and stored in the requested 64-bit register destination (using REX.W).
        64 	64 	64-bit effective address is calculated (default address size) and all 64-bits of the address are stored in the requested 64-bit register destination (using REX.W).
        */

        block->add_command(std::make_shared<cmd_context_store>(codec::reg(reg)));
    }

    bool lea::skip(const uint8_t idx)
    {
        // we skip first operand
        return idx == 0;
    }
}
