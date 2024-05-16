#pragma once
#include <cstdint>

namespace eagle::ir
{
    enum class ir_size
    {
        bit_512 = 512,
        bit_256 = 256,
        bit_128 = 128,
        bit_64 = 64,
        bit_32 = 32,
        bit_16 = 16,
        bit_8 = 8,
        none = 0,
    };
}
