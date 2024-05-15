#include "eaglevm-core/virtual_machine/machines/base_machine.h"
#include "eaglevm-core/virtual_machine/ir/block.h"

namespace eagle::virt
{
    asmb::code_container_ptr base_machine::lift_block(const ir::block_il_ptr& block, bool scatter)
    {
        const size_t command_count = block->get_command_count();

        const asmb::code_container_ptr code = asmb::code_container::create("block_begin " + command_count, true);
        for (size_t i = 0; i < command_count; i++)
        {
            const ir::base_command_ptr command = block->get_command(i);
            switch (command->get_command_type())
            {
                case ir::command_type::vm_enter:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_vm_enter>(command));
                    break;
                case ir::command_type::vm_exit:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_vm_exit>(command));
                    break;
                case ir::command_type::vm_handler_call:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_handler_call>(command));
                    break;
                case ir::command_type::vm_reg_load:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_context_load>(command));
                    break;
                case ir::command_type::vm_reg_store:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_context_store>(command));
                    break;
                case ir::command_type::vm_push:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_push>(command));
                    break;
                case ir::command_type::vm_pop:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_pop>(command));
                    break;
                case ir::command_type::vm_mem_read:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_mem_read>(command));
                    break;
                case ir::command_type::vm_mem_write:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_mem_write>(command));
                    break;
                case ir::command_type::vm_context_load:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_context_load>(command));
                    break;
                case ir::command_type::vm_exec_x86:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_x86_exec>(command));
                    break;
                case ir::command_type::vm_exec_dynamic_x86:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_x86_dynamic>(command));
                    break;
                case ir::command_type::vm_rflags_load:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_rflags_load>(command));
                    break;
                case ir::command_type::vm_rflags_store:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_rflags_store>(command));
                    break;
                case ir::command_type::vm_sx:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_sx>(command));
                    break;
                case ir::command_type::vm_branch:
                    handle_cmd(code, std::static_pointer_cast<ir::cmd_branch>(command));
                    break;
            }
        }

        return code;
    }
}
