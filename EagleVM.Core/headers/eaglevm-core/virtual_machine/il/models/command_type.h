#pragma once

namespace eagle::il
{
    enum class command_type
    {
        vm_enter,
        vm_exit,

        vm_param,
        vm_handler_call,
    };
}

