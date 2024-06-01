#include "eaglevm-core/virtual_machine/machines/eagle/machine.h"

namespace eagle::virt::eg
{
    std::vector<asmb::code_container_ptr> machine::create_handlers()
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_context_load_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_context_store_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_branch_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_handler_call_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_mem_read_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_mem_write_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_pop_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_push_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_rflags_load_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_rflags_store_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_sx_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_vm_enter_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_vm_exit_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_x86_dynamic_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_x86_exec_ptr cmd)
    {
    }
}
