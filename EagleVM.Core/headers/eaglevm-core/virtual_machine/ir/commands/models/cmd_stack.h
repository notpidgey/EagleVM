#pragma once

namespace eagle::ir
{
    enum class info_type
    {
        none,
        vm_register,
        vm_temp_register,
        immediate,
        address
    };
}