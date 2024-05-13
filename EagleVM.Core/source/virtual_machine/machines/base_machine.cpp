#include "eaglevm-core/virtual_machine/machines/base_machine.h"
#include "eaglevm-core/virtual_machine/ir/block.h"

namespace eagle::virt
{
    asmb::code_label_ptr base_machine::lift_block(const il::block_il_ptr& block, bool scatter)
    {
        const size_t command_count = block->get_command_count();

        const asmb::code_label_ptr code = asmb::code_label::create("block_begin " + command_count, true);
        for (size_t i = 0; i < command_count; i++)
        {
            const il::base_command_ptr command = block->get_command(i);
            switch (command->get_command_type())
            {
                case il::command_type::vm_enter:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_vm_enter>(command));
                    break;
                case il::command_type::vm_exit:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_vm_exit>(command));
                    break;
                case il::command_type::vm_handler_call:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_handler_call>(command));
                    break;
                case il::command_type::vm_reg_load:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_context_load>(command));
                    break;
                case il::command_type::vm_reg_store:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_context_store>(command));
                    break;
                case il::command_type::vm_push:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_push>(command));
                    break;
                case il::command_type::vm_pop:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_pop>(command));
                    break;
                case il::command_type::vm_mem_read:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_mem_read>(command));
                    break;
                case il::command_type::vm_mem_write:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_mem_write>(command));
                    break;
                case il::command_type::vm_context_load:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_context_load>(command));
                    break;
                case il::command_type::vm_exec_x86:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_x86_exec>(command));
                    break;
                case il::command_type::vm_exec_dynamic_x86:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_x86_dynamic>(command));
                    break;
                case il::command_type::vm_rflags_load:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_rflags_load>(command));
                    break;
                case il::command_type::vm_rflags_store:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_rflags_store>(command));
                    break;
                case il::command_type::vm_sx:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_sx>(command));
                    break;
                case il::command_type::vm_branch:
                    handle_cmd(code, std::static_pointer_cast<il::cmd_branch>(command));
                    break;
            }
        }

        return code;
    }

    void base_machine::add_translator(const il::command_type cmd, const lift_func& func)
    {
        lifters[cmd] = func;
    }

    lift_func base_machine::get_translator(const il::command_type cmd)
    {
        if(lifters.contains(cmd))
            return lifters[cmd];

        return nullptr;
    }

}
