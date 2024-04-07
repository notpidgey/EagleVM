#include "eaglevm-core/virtual_machine/il/commands/models/cmd_type.h"

std::string eagle::il::command_to_string(const command_type cmd)
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
