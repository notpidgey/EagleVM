#pragma once
#include <string>

namespace eagle::il
{
    enum class command_type
    {
        vm_enter,
        vm_exit,

        vm_param,
        vm_handler_call,
        vm_reg_load,
        vm_reg_store,

        vm_push,
        vm_pop
    };

    inline std::string command_to_string(const command_type cmd)
    {
        switch (cmd)
        {
            case command_type::vm_enter:
                return "EG_ENTER";
            case command_type::vm_exit:
                return "EG_EXIT";
            case command_type::vm_param:
                return "EG_PARAM";
            case command_type::vm_handler_call:
                return "EG_CALL";
            case command_type::vm_reg_load:
                return "EG_REG_LOAD";
            case command_type::vm_reg_store:
                return "EG_REG_STORE";
        }

        return "UNKNOWN";
    }
}

