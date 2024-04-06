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

        vm_context_load
    };

    inline std::string command_to_string(const command_type cmd)
    {
        switch (cmd)
        {
            case command_type::vm_enter:
                return "vm_enter";
            case command_type::vm_exit:
                return "vm_exit";
            case command_type::vm_handler_call:
                return "call";
            case command_type::vm_reg_load:
                return "reg_load";
            case command_type::vm_reg_store:
                return "reg_store";
            case command_type::vm_push:
                return "vm_push";
            case command_type::vm_pop:
                return "vm_pop";
            case command_type::vm_mem_read:
                return "read";
            case command_type::vm_mem_write:
                return "write";
        }

        return "UNKNOWN";
    }
}

