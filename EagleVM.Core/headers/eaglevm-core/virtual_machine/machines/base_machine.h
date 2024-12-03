#pragma once
#include "eaglevm-core/compiler/code_container.h"
#include "eaglevm-core/disassembler/basic_block.h"
#include "eaglevm-core/virtual_machine/ir/commands/include.h"

namespace eagle::virt
{
    using lift_func = std::function<dasm::basic_block(ir::base_command, ir::block_ptr)>;

    class base_machine
    {
    public:
        virtual ~base_machine() = default;
        virtual asmb::code_container_ptr lift_block(const ir::block_ptr& block);
        virtual std::vector<asmb::code_container_ptr> create_handlers() = 0;

        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_load_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_store_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_branch_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_handler_call_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_mem_read_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_mem_write_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_pop_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_push_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_flags_load_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_rflags_load_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_rflags_store_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_sx_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_vm_enter_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_vm_exit_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_x86_exec_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_jmp_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_and_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_or_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_xor_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_shl_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_shr_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_add_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_sub_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_cmp_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_resize_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_cnt_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_smul_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_umul_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_abs_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_log2_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_dup_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_call_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_ret_ptr& cmd) = 0;
        virtual void handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_carry_ptr& cmd) = 0;

        void add_block_context(const std::vector<ir::block_ptr>& blocks);
        void add_block_context(const ir::block_ptr& block);
        void add_block_context(const std::vector<std::pair<ir::block_ptr, asmb::code_label_ptr>>& blocks);
        void add_block_context(const ir::block_ptr& block, const asmb::code_label_ptr& label);
        void add_block_context(std::unordered_map<ir::block_ptr, asmb::code_label_ptr> block_map);

        [[nodiscard]] std::vector<std::pair<ir::block_ptr, asmb::code_label_ptr>> get_blocks() const;

    protected:
        std::unordered_map<ir::block_ptr, asmb::code_label_ptr> block_context;

        virtual void dispatch_handle_cmd(const asmb::code_container_ptr& code, const ir::base_command_ptr& command);

        codec::mnemonic to_jump_mnemonic(ir::exit_condition condition);
        asmb::code_label_ptr get_block_label(const ir::block_ptr& block);
    };
}
