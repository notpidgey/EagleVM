#pragma once
#include "eaglevm-core/virtual_machine/machines/pidgeon/settings.h"
#include "eaglevm-core/virtual_machine/machines/pidgeon/inst_regs.h"
#include "eaglevm-core/virtual_machine/machines/pidgeon/inst_handlers.h"

#include "eaglevm-core/virtual_machine/machines/base_machine.h"
#include "eaglevm-core/virtual_machine/machines/register_context.h"

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


    class machine final : public base_machine, public std::enable_shared_from_this<machine>
    {
    public:
        machine(const settings_ptr& settings_info);
        static machine_ptr create(const settings_ptr& settings_info);

        std::vector<asmb::code_container_ptr> create_handlers() override;

        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_load_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_store_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_branch_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_handler_call_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_mem_read_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_mem_write_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_pop_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_push_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_rflags_load_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_rflags_store_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_sx_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_vm_enter_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_vm_exit_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_x86_exec_ptr& cmd) override;

    private:
        register_context_ptr transaction;

        settings_ptr settings;

        vm_inst_regs_ptr rm;
        vm_inst_handlers_ptr hg;

        void dispatch_handle_cmd(const asmb::code_container_ptr& code, const ir::base_command_ptr& command) override;
        codec::reg reg_vm_to_register(ir::reg_vm store) const;
    };
}
