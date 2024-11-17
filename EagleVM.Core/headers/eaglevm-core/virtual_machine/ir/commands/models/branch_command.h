#pragma once
#include <variant>

namespace eagle::ir
{
    using block_ptr = std::shared_ptr<class block_ir>;
    using ir_exit_result = std::variant<uint64_t, block_ptr>;

    class branch_command
    {
    public:
        void rewrite_branch(const ir_exit_result& search, const ir_exit_result& target)
        {
            auto check_block = [&](ir_exit_result& exit_result)
            {
                if (search == exit_result)
                    exit_result = target;

                return false;
            };

            for (auto& branch : branches)
                check_block(branch);
        }

        bool branch_visits(const uint64_t rva) const
        {
            auto check_block = [&](const ir_exit_result& exit_result) -> bool
            {
                if (std::holds_alternative<uint64_t>(exit_result))
                {
                    const uint64_t exit_rva = std::get<uint64_t>(exit_result);
                    return exit_rva == rva;
                }

                return false;
            };

            for (auto& branch : branches)
                if (check_block(branch))
                    return true;

            return false;
        }

        bool branch_visits(const block_ptr& block) const
        {
            auto check_block = [&](const ir_exit_result& exit_result) -> bool
            {
                if (std::holds_alternative<block_ptr>(exit_result))
                {
                    const block_ptr exit_block = std::get<block_ptr>(exit_result);
                    return exit_block == block;
                }

                return false;
            };

            for (auto& branch : branches)
                if (check_block(branch))
                    return true;

            return false;
        }

        std::vector<ir_exit_result> get_branches()
        {
            return branches;
        }

    protected:
        std::vector<ir_exit_result> branches;
    };

    using branch_command_ptr = std::shared_ptr<class branch_command>;
}
