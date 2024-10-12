#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    command_type base_command::get_command_type() const
    {
        return command;
    }

    std::shared_ptr<base_command> base_command::release(const std::vector<discrete_store_ptr>& stores)
    {
        release_list.append_range(stores);
        return shared_from_this();
    }

    std::shared_ptr<base_command> base_command::release(const discrete_store_ptr& store)
    {
        release_list.push_back(store);
        return shared_from_this();
    }

    std::vector<discrete_store_ptr> base_command::get_use_stores()
    {
        return { };
    }

    std::string base_command::command_to_string(const command_type type)
    {
        switch (type)
        {
            case command_type::vm_enter:
                return "vm_enter";
            case command_type::vm_exit:
                return "vm_exit";
            case command_type::vm_handler_call:
                return "vm_handler_call";
            case command_type::vm_reg_load:
                return "vm_reg_load";
            case command_type::vm_reg_store:
                return "vm_reg_store";
            case command_type::vm_push:
                return "vm_push";
            case command_type::vm_pop:
                return "vm_pop";
            case command_type::vm_mem_read:
                return "vm_mem_read";
            case command_type::vm_mem_write:
                return "vm_mem_write";
            case command_type::vm_context_load:
                return "vm_context_load";
            case command_type::vm_context_store:
                return "vm_context_store";
            case command_type::vm_exec_x86:
                return "vm_exec_x86";
            case command_type::vm_exec_dynamic_x86:
                return "vm_exec_dynamic_x86";
            case command_type::vm_rflags_load:
                return "vm_rflags_load";
            case command_type::vm_rflags_store:
                return "vm_rflags_store";
            case command_type::vm_sx:
                return "vm_sx";
            case command_type::vm_branch:
                return "vm_branch";
            default:
                return "unknown";
        }
    }
}
