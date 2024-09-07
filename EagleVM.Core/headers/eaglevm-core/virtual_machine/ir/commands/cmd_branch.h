#pragma once
#include <array>
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    enum class exit_location
    {
        none,
        block,
        rva
    };

    enum class exit_condition
    {
        none,
        jb,
        jbe,
        jcxz,
        jecxz,
        jknzd,
        jkzd,
        jl,
        jle,
        jmp,
        jnb,
        jnbe,
        jnl,
        jnle,
        jno,
        jnp,
        jns,
        jnz,
        jo,
        jp,
        jrcxz,
        js,
        jz,
    };

    class block_ir;
    using block_ptr = std::shared_ptr<block_ir>;

    using vmexit_rva = uint64_t;
    using il_exit_result = std::variant<vmexit_rva, block_ptr>;

    class cmd_branch : public base_command
    {
    public:
        cmd_branch(const il_exit_result& result_info, exit_condition condition);
        cmd_branch(const std::vector<il_exit_result>& result_info, exit_condition condition);

        [[nodiscard]] exit_condition get_condition() const;
        il_exit_result& get_condition_default();
        il_exit_result& get_condition_special();

        bool branch_visits(vmexit_rva rva);
        bool branch_visits(const block_ptr& block);

        void rewrite_branch(const il_exit_result& search, const il_exit_result& target);
        bool is_similar(const std::shared_ptr<base_command>& other) override;

    private:
        std::vector<il_exit_result> info;
        exit_condition condition;
    };
}
