#include "eaglevm-core/virtual_machine/ir/commands/cmd_branch.h"

namespace eagle::ir
{
    cmd_branch::cmd_branch(const ir_exit_result& result_fallthrough) :
        base_command(command_type::vm_branch), condition(exit_condition::jmp), invert_condition(false), virtual_branch(false)
    {
        info.push_back(result_fallthrough);
    }

    cmd_branch::cmd_branch(const ir_exit_result& result_fallthrough, const ir_exit_result& result_conditional, const exit_condition condition,
        const bool invert_condition) :
        base_command(command_type::vm_branch), condition(condition), invert_condition(invert_condition), virtual_branch(false)
    {
        info.reserve(2);
        info.push_back(result_fallthrough);
        info.push_back(result_conditional);
    }

    exit_condition cmd_branch::get_condition() const { return condition; }

    ir_exit_result& cmd_branch::get_condition_default()
    {
        // the default condition which is always a fall through to the next bb or jmp
        // will be at the back
        return info.back();
    }

    ir_exit_result& cmd_branch::get_condition_special() { return info.front(); }

    bool cmd_branch::branch_visits(const uint64_t rva)
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

        return check_block(get_condition_default()) || check_block(get_condition_special());
    }

    bool cmd_branch::branch_visits(const block_ptr& block)
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

        return check_block(get_condition_default()) || check_block(get_condition_special());
    }

    void cmd_branch::rewrite_branch(const ir_exit_result& search, const ir_exit_result& target)
    {
        auto check_block = [&](ir_exit_result& exit_result)
        {
            if (search == exit_result)
                exit_result = target;

            return false;
        };

        check_block(get_condition_default());
        check_block(get_condition_special());
    }

    bool cmd_branch::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_branch>(other);
        return condition == cmd->get_condition() && get_condition_default() == cmd->get_condition_default() &&
            get_condition_special() == cmd->get_condition_special() && base_command::is_similar(other);
    }

    void cmd_branch::set_virtual(const bool is_virtual) { this->virtual_branch = is_virtual; }

    bool cmd_branch::is_virtual() const { return virtual_branch; }
    bool cmd_branch::is_inverted() { return invert_condition; }

    std::string cmd_branch::to_string()
    {
        return base_command::to_string() + (virtual_branch ? " (virtual)" : " (native)");
    }
}
