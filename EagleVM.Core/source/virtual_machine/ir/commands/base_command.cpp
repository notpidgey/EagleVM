#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    base_command::base_command(const command_type command, const bool force_inline): type(command), force_inline(force_inline)
    {
        static uint32_t id = 0;

        unique_id = id;
        unique_id_string = command_to_string(command) + ": " + std::to_string(id++);
    }

    command_type base_command::get_command_type() const
    {
        return type; }

    void base_command::set_inlined(const bool inlined) { force_inline = inlined; };
    bool base_command::is_inlined() const { return force_inline; }

    std::vector<discrete_store_ptr> base_command::get_use_stores()
    {
        return { };
    }

    bool base_command::is_similar(const std::shared_ptr<base_command>& other)
    {
        return other->get_command_type() == get_command_type();
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
            case command_type::vm_context_rflags_load:
                return "vm_rflags_load";
            case command_type::vm_context_rflags_store:
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
