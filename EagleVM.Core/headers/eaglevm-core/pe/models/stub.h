#pragma once
#include <cstdint>

namespace eagle::pe
{
    enum class stub_import : uint8_t
    {
        unknown = 0,
        vm_begin = 1,
        vm_end = 2,
    };
}