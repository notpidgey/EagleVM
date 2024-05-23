#pragma once

namespace eagle::ir
{
    enum class stack_type
    {
        none,
        vm_register,
        vm_temp_register,
        immediate
    };
}