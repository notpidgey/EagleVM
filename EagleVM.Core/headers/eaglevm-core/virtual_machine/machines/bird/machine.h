#pragma once
#include "eaglevm-core/virtual_machine/machines/base_machine.h"
#include <vector>

#include "eaglevm-core/codec/zydis_encoder.h"
#include "eaglevm-core/virtual_machine/machines/eagle/register_manager.h"
#include "eaglevm-core/virtual_machine/machines/eagle/settings.h"

namespace eagle::virt
{
    using register_context_ptr = std::shared_ptr<class register_context>;
}

namespace eagle::virt::eg
{
    using bird_machine_ptr = std::shared_ptr<class bird_machine>;
    class bird_machine final : public base_machine
    {
    public:
        explicit bird_machine(const settings_ptr& settings_info);
        static bird_machine_ptr create(const settings_ptr& settings_info);

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
        codec::encoder::encode_builder_ptr out_block;

        settings_ptr settings;
        register_manager_ptr reg_man;

        register_context_ptr reg_64_container;
        register_context_ptr reg_128_container;

        [[nodiscard]] codec::reg reg_vm_to_register(ir::reg_vm store) const;
        void handle_generic_logic_cmd(codec::mnemonic command, ir::ir_size size, bool preserved);

        void pop_to_register(codec::reg register);
        void push_register(codec::reg register);

        void create_handler(bool force_inline, std::function<void(codec::encoder::encode_builder_ptr&, std::function<codec::reg()>)> handler_create);
    };
}
