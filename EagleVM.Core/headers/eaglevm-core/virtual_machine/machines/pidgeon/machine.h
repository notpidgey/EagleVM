#pragma once
#include "eaglevm-core/virtual_machine/machines/pidgeon/inst_regs.h"
#include "eaglevm-core/virtual_machine/machines/pidgeon/inst_handlers.h"

#include "eaglevm-core/virtual_machine/machines/base_machine.h"

namespace eagle::virt::pidg
{
    // todo: i want to add options for inlining vm handlers such as pop
    // i want a lot of control over this such as being able to randomly inline

    // enum vm_inline_opt
    // {
    //     inline_none,
    //     inline_x86_handlers,
    //     inline_vm_handlers,
    // };

    // i need to do a rule checker
    // some rules to keep in mind
    // gpr registers only, no xmm, no avx, nothing like that

    class machine final : public base_machine
    {
    public:
        machine(bool variant_handlers);

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

        void create_vm_jump(codec::mnemonic mnemonic, const asmb::code_container_ptr& container, const asmb::code_container_ptr& rva_target);

        codec::encoded_vec create_jump(uint32_t rva, const asmb::code_container_ptr& rva_target);

    private:
        vm_inst_regs_ptr rm_;
        vm_inst_handlers_ptr hg_;

        bool variant_register_handlers;

        void call_handler(const asmb::code_container_ptr& code, const asmb::code_label_ptr& target);
    };
}
