#pragma once
#include "eaglevm-core/virtual_machine/machines/base_machine.h"

namespace eagle::virt::eg
{
    class machine : public base_machine
    {
    public:
        std::vector<asmb::code_container_ptr> create_handlers() override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_context_load_ptr cmd) override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_context_store_ptr cmd) override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_branch_ptr cmd) override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_handler_call_ptr cmd) override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_mem_read_ptr cmd) override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_mem_write_ptr cmd) override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_pop_ptr cmd) override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_push_ptr cmd) override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_rflags_load_ptr cmd) override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_rflags_store_ptr cmd) override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_sx_ptr cmd) override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_vm_enter_ptr cmd) override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_vm_exit_ptr cmd) override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_x86_dynamic_ptr cmd) override;
        void handle_cmd(asmb::code_container_ptr block, ir::cmd_x86_exec_ptr cmd) override;
    };
}
