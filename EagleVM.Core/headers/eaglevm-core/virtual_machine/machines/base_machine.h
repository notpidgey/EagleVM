#pragma once
#include "eaglevm-core/compiler/code_label.h"
#include "eaglevm-core/disassembler/basic_block.h"
#include "eaglevm-core/virtual_machine/ir/commands/include.h"

namespace eagle::virt
{
    using lift_func = std::function<dasm::basic_block(il::base_command, il::block_il_ptr)>;

    class base_machine
    {
    public:
        virtual ~base_machine() = default;
        virtual asmb::code_label_ptr lift_block(const il::block_il_ptr& block, bool scatter);
        virtual std::vector<asmb::code_label_ptr> create_handlers() = 0;

        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_context_load_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_context_store_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_branch_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_handler_call_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_mem_read_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_mem_write_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_pop_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_push_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_rflags_load_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_rflags_store_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_sx_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_vm_enter_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_vm_exit_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_x86_dynamic_ptr cmd) = 0;
        virtual void handle_cmd(asmb::code_label_ptr label, il::cmd_x86_exec_ptr cmd) = 0;

    protected:
        std::unordered_map<il::command_type, lift_func> lifters;

        void add_translator(il::command_type, const lift_func&);
        lift_func get_translator(il::command_type);
    };
}