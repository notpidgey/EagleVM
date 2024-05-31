#pragma once
#include <cstdint>

namespace eagle::pe
{
    constexpr uint8_t signature_size = 4;
    constexpr uint8_t guid_size = 16;

    struct code_view_pdb
    {
        uint8_t signature[signature_size];
        uint8_t guid[guid_size];
        uint32_t age;
    };
}