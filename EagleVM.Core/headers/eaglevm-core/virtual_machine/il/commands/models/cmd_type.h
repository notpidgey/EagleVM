#pragma once
#include <string>

namespace eagle::il
{
    enum class command_type
    {
        vm_enter,
        vm_exit,

        vm_handler_call,
        vm_reg_load,
        vm_reg_store,

        vm_push,
        vm_pop,

        vm_mem_read,
        vm_mem_write,

        vm_context_load,
        vm_exec_x86
    };

    std::string command_to_string(command_type cmd);
}

