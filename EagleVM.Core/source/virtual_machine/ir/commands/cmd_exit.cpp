#include "eaglevm-core/virtual_machine/ir/commands/cmd_vm_exit.h"
#include "eaglevm-core/virtual_machine/ir/block.h"

namespace eagle::ir
{
    cmd_vm_exit::cmd_vm_exit(const ir_exit_result& result)
        : base_command(command_type::vm_exit)
    {
        branches.push_back(result);
    }

    template<class... Ts>
    struct overloaded : Ts... { using Ts::operator()...; };

    bool cmd_vm_exit::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_vm_exit>(other);
        return base_command::is_similar(other) && branches[0].index() == cmd->branches[0].index() &&
            std::visit(overloaded{
                [](const uint64_t a, const uint64_t b) { return a == b; },
                [](const block_ptr& a, const block_ptr& b) { return a == b; },
                [](auto a, auto b) { return false; },
            }, cmd->branches[0], branches[0]);
    }

    ir_exit_result cmd_vm_exit::get_exit()
    {
        return branches.front();
    }

    std::string cmd_vm_exit::to_string()
    {
        std::string out;
        if (std::holds_alternative<block_ptr>(branches[0]))
        {
            const auto cmd = std::get<block_ptr>(branches[0]);
            out += std::format(" block 0x{:x}", cmd->block_id);
        }
        else
        {
            out += std::format(" OUT 0x{:x}",  std::get<uint64_t>(branches[0]));
        }

        return base_command::to_string() + out;
    }
}
