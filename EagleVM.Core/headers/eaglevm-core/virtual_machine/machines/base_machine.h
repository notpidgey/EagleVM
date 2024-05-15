#pragma once
#include "eaglevm-core/compiler/code_container.h"
#include "eaglevm-core/disassembler/basic_block.h"
#include "eaglevm-core/virtual_machine/ir/commands/include.h"

namespace eagle::virt
{
    using lift_func = std::function<dasm::basic_block(ir::base_command, ir::block_il_ptr)>;

    class base_machine
    {
    public:
        virtual ~base_machine() = default;
        virtual asmb::code_container_ptr lift_block(const ir::block_il_ptr& block, bool scatter);
        virtual std::vector<asmb::code_container_ptr> create_handlers() = 0;

        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_context_load_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_context_store_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_branch_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_handler_call_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_mem_read_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_mem_write_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_pop_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_push_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_rflags_load_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_rflags_store_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_sx_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_vm_enter_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_vm_exit_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_x86_dynamic_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_container_ptr label, ir::cmd_x86_exec_ptr cmd) = 0;
    };
}