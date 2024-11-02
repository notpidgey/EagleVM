#include "eaglevm-core/virtual_machine/ir/commands/cmd_branch.h"
#include "eaglevm-core/virtual_machine/ir/block.h"

namespace eagle::ir
{
    cmd_branch::cmd_branch(const ir_exit_result& result_fallthrough) :
        base_command(command_type::vm_branch), condition(exit_condition::jmp), invert_condition(false), virtual_branch(false)
    {
        branches.push_back(result_fallthrough);
    }

    cmd_branch::cmd_branch(const ir_exit_result& result_fallthrough, const ir_exit_result& result_conditional, const exit_condition condition,
        const bool invert_condition) :
        base_command(command_type::vm_branch), condition(condition), invert_condition(invert_condition), virtual_branch(false)
    {
        branches.reserve(2);
        branches.push_back(result_fallthrough);
        branches.push_back(result_conditional);
    }

    exit_condition cmd_branch::get_condition() const { return condition; }

    ir_exit_result& cmd_branch::get_condition_default()
    {
        // the default condition which is always a fall through to the next bb or jmp
        // will be at the back
        return branches.back();
    }

    ir_exit_result& cmd_branch::get_condition_special() { return branches.front(); }

    bool cmd_branch::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_branch>(other);
        return condition == cmd->get_condition() && get_condition_default() == cmd->get_condition_default() &&
            get_condition_special() == cmd->get_condition_special() && base_command::is_similar(other);
    }

    void cmd_branch::set_virtual(const bool is_virtual) { this->virtual_branch = is_virtual; }

    bool cmd_branch::is_virtual() const { return virtual_branch; }
    bool cmd_branch::is_inverted() const { return invert_condition; }

    std::string cmd_branch::to_string()
    {
        std::string out;
        for (const ir_exit_result& branch : branches)
        {
            if (std::holds_alternative<block_ptr>(branch))
            {
                const auto cmd = std::get<block_ptr>(branch);
                out += " block " + std::to_string(cmd->block_id) + ",";
            }
            else
                out += " ?,";
        }

        out.pop_back();

        return base_command::to_string() + out +  (virtual_branch ? " (virtual)" : " (native)");
    }
}
