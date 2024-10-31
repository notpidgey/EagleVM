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

        asmb::code_container_ptr lift_block(const ir::block_ptr& block) override;
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
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_x86_dynamic_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_x86_exec_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_flags_load_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_jmp_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_and_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_or_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_xor_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_shl_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_shr_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_add_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_sub_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_cmp_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_resize_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_cnt_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_smul_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_umul_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_abs_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_log2_ptr& cmd) override;
        void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_dup_ptr& cmd) override;

        std::vector<asmb::code_container_ptr> create_handlers() override;

    private:
        settings_ptr settings;
        register_manager_ptr reg_man;
        inst_handlers_ptr han_man;

        register_context_ptr reg_64_container;
        register_context_ptr reg_128_container;

        std::unordered_map<ir::discrete_store_ptr, complex_load_info> store_complex_load_info;

        void dispatch_handle_cmd(const asmb::code_container_ptr& code, const ir::base_command_ptr& command) override;

        void call_push(const asmb::code_container_ptr& block, const ir::discrete_store_ptr& shared) const;
        void call_push(const asmb::code_container_ptr& block, codec::reg target_reg) const;

        void call_pop(const asmb::code_container_ptr& block, const ir::discrete_store_ptr& shared) const;
        void call_pop(const asmb::code_container_ptr& block, const ir::discrete_store_ptr& shared, codec::reg_size size) const;
        void call_pop(const asmb::code_container_ptr& block, codec::reg target_reg) const;

        [[nodiscard]] codec::reg reg_vm_to_register(ir::reg_vm store) const;

        void handle_generic_logic_cmd(const asmb::code_container_ptr& block, codec::mnemonic command, ir::ir_size size, bool preserved);
    };
}
