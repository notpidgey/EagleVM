#include "eaglevm-core/virtual_machine/ir/x86/util.h"

namespace eagle::ir
{
    ir_size ir::bits_to_ir_size(const uint16_t bit_count)
    {
        switch (bit_count)
        {
            case 0:
                return ir_size::none;
            case 512:
                return ir_size::bit_512;
            case 256:
                return ir_size::bit_256;
            case 128:
                return ir_size::bit_128;
            case 64:
                return ir_size::bit_64;
            case 32:
                return ir_size::bit_32;
            case 16:
                return ir_size::bit_16;
            case 8:
                return ir_size::bit_8;
            default:
            {
                assert("reached invalid bit size of ir size");
                return ir_size::none;
            }
        }
    }
}
