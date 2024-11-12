#pragma once
#include <cstdint>
#include <string>

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

    inline std::string ir_size_to_string(const ir_size size)
    {
        switch(size)
        {
            case ir_size::bit_512:
                return "512-bit";
            case ir_size::bit_256:
                return "256-bit";
            case ir_size::bit_128:
                return "128-bit";
            case ir_size::bit_64:
                return "64-bit";
            case ir_size::bit_32:
                return "32-bit";
            case ir_size::bit_16:
                return "16-bit";
            case ir_size::bit_8:
                return "8-bit";
            case ir_size::none:
                return "none";
            default:
                return "unknown";
        }
    }
}
