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
}