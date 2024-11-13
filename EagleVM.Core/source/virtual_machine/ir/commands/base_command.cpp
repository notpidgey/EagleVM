#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    base_command::base_command(const command_type command, const bool force_inline): type(command), force_inline(force_inline)
    {
        static uint32_t id = 0;

        unique_id = id;
        unique_id_string = cmd_type_to_string(command) + ": " + std::to_string(id++);
    }

    command_type base_command::get_command_type() const
    {
        return type;
    }

    void base_command::set_inlined(const bool inlined) { force_inline = inlined; };
    bool base_command::is_inlined() const { return force_inline; }

    bool base_command::is_similar(const std::shared_ptr<base_command>& other)
    {
        return other->get_command_type() == get_command_type();
    }

    std::string base_command::cmd_type_to_string(const command_type type)
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
            case command_type::vm_context_rflags_load:
                return "vm_rflags_load";
            case command_type::vm_context_rflags_store:
                return "vm_rflags_store";
            case command_type::vm_sx:
                return "vm_sx";
            case command_type::vm_branch:
                return "vm_branch";
            case command_type::none:
                return "none";
            case command_type::vm_flags_load:
                return "vm_flags_load";
            case command_type::vm_resize:
                return "vm_resize";
            case command_type::vm_jmp:
                return "vm_jmp";
            case command_type::vm_and:
                return "vm_and";
            case command_type::vm_or:
                return "vm_or";
            case command_type::vm_xor:
                return "vm_xor";
            case command_type::vm_shl:
                return "vm_shl";
            case command_type::vm_shr:
                return "vm_shr";
            case command_type::vm_cnt:
                return "vm_cnt";
            case command_type::vm_add:
                return "vm_add";
            case command_type::vm_sub:
                return "vm_sub";
            case command_type::vm_smul:
                return "vm_smul";
            case command_type::vm_umul:
                return "vm_umul";
            case command_type::vm_abs:
                return "vm_abs";
            case command_type::vm_log2:
                return "vm_log2";
            case command_type::vm_dup:
                return "vm_dup";
            case command_type::vm_cmp:
                return "vm_cmp";
            case command_type::vm_call:
                return "vm_call";
            case command_type::vm_ret:
                return "vm_ret";
            default:
                return "unknown";
        }
    }

    std::string base_command::to_string()
    {
        return cmd_type_to_string(type);
    }
}
