#include <vector>
#include <memory>
#include "eaglevm-core/virtual_machine/ir/commands/include.h"

namespace eagle::ir
{
    class block_builder
    {
    public:
        block_builder& add_mem_read(ir_size size)
        {
            commands.push_back(std::make_shared<cmd_mem_read>(size));
            return *this;
        }

        block_builder& add_cmp(ir_size size)
        {
            commands.push_back(std::make_shared<cmd_cmp>(size));
            return *this;
        }

        block_builder& add_context_load(codec::reg source)
        {
            commands.push_back(std::make_shared<cmd_context_load>(source));
            return *this;
        }

        block_builder& add_context_store(codec::reg dest)
        {
            commands.push_back(std::make_shared<cmd_context_store>(dest));
            return *this;
        }

        block_builder& add_context_store(codec::reg dest, codec::reg_size size)
        {
            commands.push_back(std::make_shared<cmd_context_store>(dest, size));
            return *this;
        }

        block_builder& add_context_rflags_store(x86_cpu_flag relevant_flags)
        {
            commands.push_back(std::make_shared<cmd_context_rflags_store>(relevant_flags));
            return *this;
        }

        block_builder& add_context_rflags_load()
        {
            commands.push_back(std::make_shared<cmd_context_rflags_load>());
            return *this;
        }

        block_builder& add_jmp()
        {
            commands.push_back(std::make_shared<cmd_jmp>());
            return *this;
        }

        block_builder& add_flags_load(vm_flags flag)
        {
            commands.push_back(std::make_shared<cmd_flags_load>(flag));
            return *this;
        }

        block_builder& add_and(ir_size size, bool reversed = false, bool preserve_args = false)
        {
            commands.push_back(std::make_shared<cmd_and>(size, reversed, preserve_args));
            return *this;
        }

        block_builder& add_or(ir_size size, bool reversed = false, bool preserve_args = false)
        {
            commands.push_back(std::make_shared<cmd_or>(size, reversed, preserve_args));
            return *this;
        }

        block_builder& add_xor(ir_size size, bool reversed = false, bool preserve_args = false)
        {
            commands.push_back(std::make_shared<cmd_xor>(size, reversed, preserve_args));
            return *this;
        }

        block_builder& add_shl(ir_size size, bool reversed = false, bool preserve_args = false)
        {
            commands.push_back(std::make_shared<cmd_shl>(size, reversed, preserve_args));
            return *this;
        }

        block_builder& add_shr(ir_size size, bool reversed = false, bool preserve_args = false)
        {
            commands.push_back(std::make_shared<cmd_shr>(size, reversed, preserve_args));
            return *this;
        }

        block_builder& add_cnt(ir_size size, bool reversed = false, bool preserve_args = false)
        {
            commands.push_back(std::make_shared<cmd_cnt>(size, reversed, preserve_args));
            return *this;
        }

        block_builder& add_add(ir_size size, bool reversed = false, bool preserve_args = false)
        {
            commands.push_back(std::make_shared<cmd_add>(size, reversed, preserve_args));
            return *this;
        }

        block_builder& add_sub(ir_size size, bool reversed = false, bool preserve_args = false)
        {
            commands.push_back(std::make_shared<cmd_sub>(size, reversed, preserve_args));
            return *this;
        }

        block_builder& add_smul(ir_size size, bool reversed = false, bool preserve_args = false)
        {
            commands.push_back(std::make_shared<cmd_smul>(size, reversed, preserve_args));
            return *this;
        }

        block_builder& add_umul(ir_size size, bool reversed = false, bool preserve_args = false)
        {
            commands.push_back(std::make_shared<cmd_umul>(size, reversed, preserve_args));
            return *this;
        }

        block_builder& add_abs(ir_size size, bool preserve_args = false)
        {
            commands.push_back(std::make_shared<cmd_abs>(size, preserve_args));
            return *this;
        }

        block_builder& add_log2(ir_size size, bool preserve_args = false)
        {
            commands.push_back(std::make_shared<cmd_log2>(size, preserve_args));
            return *this;
        }

        block_builder& add_dup(ir_size size, bool preserve_args = false)
        {
            commands.push_back(std::make_shared<cmd_dup>(size, preserve_args));
            return *this;
        }

        block_builder& add_handler_call(codec::mnemonic mnemonic, x86_operand_sig signature)
        {
            commands.push_back(std::make_shared<cmd_handler_call>(mnemonic, signature));
            return *this;
        }

        block_builder& add_handler_call(codec::mnemonic mnemonic, handler_sig signature)
        {
            commands.push_back(std::make_shared<cmd_handler_call>(mnemonic, signature));
            return *this;
        }

        block_builder& add_branch(const ir_exit_result& result_fallthrough)
        {
            commands.push_back(std::make_shared<cmd_branch>(result_fallthrough));
            return *this;
        }

        block_builder& add_branch(const ir_exit_result& result_fallthrough, const ir_exit_result& result_conditional, exit_condition condition, bool invert_condition)
        {
            commands.push_back(std::make_shared<cmd_branch>(result_fallthrough, result_conditional, condition, invert_condition));
            return *this;
        }

        block_builder& add_push(uint64_t immediate, ir_size stack_disp)
        {
            commands.push_back(std::make_shared<cmd_push>(immediate, stack_disp));
            return *this;
        }

        block_builder& add_pop(ir_size size)
        {
            commands.push_back(std::make_shared<cmd_pop>(size));
            return *this;
        }

        block_builder& add_resize(ir_size from, ir_size to)
        {
            commands.push_back(std::make_shared<cmd_resize>(from, to));
            return *this;
        }

        block_builder& append(block_builder& other)
        {
            commands.append_range(other.build());
            return *this;
        }

        block_builder& append(std::vector<base_command_ptr> other)
        {
            commands.append_range(other);
            return *this;
        }

        std::vector<std::shared_ptr<base_command>> build()
        {
            return commands;
        }

    private:
        std::vector<std::shared_ptr<base_command>> commands;
    };
}