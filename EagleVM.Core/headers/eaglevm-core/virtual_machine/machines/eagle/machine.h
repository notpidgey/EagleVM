#pragma once
#include "eaglevm-core/virtual_machine/machines/base_machine.h"
#include <vector>

#include "handler_manager.h"
#include "register_manager.h"
#include "settings.h"

namespace eagle::virt
{
    using register_context_ptr = std::shared_ptr<class register_context>;
}

/*
 * daax inspired vm 💞
 */
namespace eagle::virt::eg
{
    using machine_ptr = std::shared_ptr<class machine>;

    class machine final : public base_machine
    {
    public:
        explicit machine(const settings_ptr& settings_info);
        static machine_ptr create(const settings_ptr& settings_info);

        virtual asmb::code_container_ptr lift_block(const ir::block_ptr& block) override;
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
        register_context_ptr reg_ctx;

        settings_ptr settings;
        register_manager_ptr reg_man;
        inst_handlers_ptr han_man;

        void handle_cmd(const asmb::code_container_ptr& code, const ir::base_command_ptr& command) override;

        void call_push(const asmb::code_container_ptr& block, const ir::discrete_store_ptr& shared);
        void call_push(const asmb::code_container_ptr& block, const codec::reg target_reg);

        void call_pop(const asmb::code_container_ptr& block, const ir::discrete_store_ptr& shared) const;
        void call_pop(const asmb::code_container_ptr& block, const codec::reg target_reg) const;

        codec::reg reg_vm_to_register(ir::reg_vm store) const;
    };
}
