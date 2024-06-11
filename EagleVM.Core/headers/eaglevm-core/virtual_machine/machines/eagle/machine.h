#pragma once
#include <complex.h>

#include "eaglevm-core/virtual_machine/machines/base_machine.h"
#include <vector>

#include "inst_handlers.h"
#include "inst_regs.h"
#include "settings.h"
#include "eaglevm-core/virtual_machine/machines/transaction_handler.h"

/*
 * daax inspired vm 💞
 */
namespace eagle::virt::eg
{
    using machine_ptr = std::shared_ptr<class machine>;
    class machine : public base_machine
    {
    public:
        explicit machine(const settings_ptr& settings_info);
        static machine_ptr create(const settings_ptr& settings_info);

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

        std::vector<asmb::code_container_ptr> create_handlers() override;

    private:
        transaction_handler_ptr reg_man;

        settings_ptr settings;
        inst_regs_ptr rm;
        inst_handlers_ptr handlers;

        void handle_cmd(const asmb::code_container_ptr& code, const ir::base_command_ptr& command) override;
    };
}
