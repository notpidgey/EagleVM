#pragma once

namespace eagle::il
{
    enum class stack_type
    {
        none,
        vm_register,
        x86_register,
        immediate
    };

    enum class stack_disp
    {
        bit_64 = 8,
        bit_32 = 4,
        bit_16 = 2,
        bit_8 = 1,
    };
}