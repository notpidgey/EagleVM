#pragma once
#include <cstdint>

namespace eagle::il
{
    enum il_size
    {
        none = 0,
        byte = 1,
        word = 2,
        dword = 4,
        qword = 8
    };

    inline il_size size_from_bits(const uint8_t bits)
    {
        switch (bits)
        {
            case 8:
                return byte;
            case 16:
                return word;
            case 32:
                return dword;
            case 64:
                return qword;
            default:
                // todo implemenet exception throwing
                return none;
        }
    }
}
