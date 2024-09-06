#include "eaglevm-core/virtual_machine/ir/x86/handlers/lea.h"

namespace eagle::ir::handler
{
    /*
     * little confused and havent fully thought out the coimplexitiy of lea
     * see 64-bit mode chart on https://www.felixcloutier.com/x86/lea
     * the way that the memory operand is calculated is not correct to these rows
     * need to fix !!!!!!
     */
    lea::lea()
    {
        valid_operands = {
            { { { codec::op_reg, codec::bit_16 }, { codec::op_mem, codec::bit_32 } }, "lea 16,16" },
            { { { codec::op_reg, codec::bit_16 }, { codec::op_mem, codec::bit_64 } }, "lea 16,16" },

            { { { codec::op_reg, codec::bit_32 }, { codec::op_mem, codec::bit_32 } }, "lea 32,32" },
            { { { codec::op_reg, codec::bit_32 }, { codec::op_mem, codec::bit_64 } }, "lea 32,32" },

            // todo: this needs to be zero extendd!!
            { { { codec::op_reg, codec::bit_64 }, { codec::op_mem, codec::bit_32 } }, "lea 64,64" },
            { { { codec::op_reg, codec::bit_64 }, { codec::op_mem, codec::bit_64 } }, "lea 64,64" },
        };

        // again, we dont need handlers for lea :)
    }
}

namespace eagle::ir::lifter
{
    translate_mem_result lea::translate_mem_action(const codec::dec::op_mem& op_mem, uint8_t idx)
    {
        return translate_mem_result::address;
    }

    void lea::finalize_translate_to_virtual()
    {
        // because of our LEA implementation we do not want to call a handler
        // a handler would serve no purpose the way this IL is setup

        // at this point we have the [] memory operand as an address on the stack
        // all we need to do is write it to the register

        codec::zydis_register reg = operands[0].reg.value;

        /*
        16 	32 	32-bit effective address is calculated (using 67H prefix). The lower 16 bits of the address are stored in the requested 16-bit register destination (using 66H prefix).
        16 	64 	64-bit effective address is calculated (default address size). The lower 16 bits of the address are stored in the requested 16-bit register destination (using 66H prefix).
        32 	32 	32-bit effective address is calculated (using 67H prefix) and stored in the requested 32-bit register destination.
        32 	64 	64-bit effective address is calculated (default address size) and the lower 32 bits of the address are stored in the requested 32-bit register destination.
        64 	32 	32-bit effective address is calculated (using 67H prefix), zero-extended to 64-bits, and stored in the requested 64-bit register destination (using REX.W).
        64 	64 	64-bit effective address is calculated (default address size) and all 64-bits of the address are stored in the requested 64-bit register destination (using REX.W).
        */

        block->push_back(std::make_shared<cmd_context_store>(static_cast<codec::reg>(reg)));
    }

    bool lea::skip(const uint8_t idx)
    {
        // we skip first operand
        return idx == 0;
    }
}
