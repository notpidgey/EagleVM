#include "eaglevm-core/virtual_machine/ir/commands/cmd_vm_exit.h"
#include "eaglevm-core/virtual_machine/ir/block.h"

namespace eagle::ir
{
    cmd_vm_exit::cmd_vm_exit(const ir_exit_result& result)
        : base_command(command_type::vm_exit)
    {
        branches.push_back(result);
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
            out += " block " + std::to_string(cmd->block_id);
        }
        else
        {
            out += " " + std::get<uint64_t>(branches[0]);
        }

        return base_command::to_string() + out;
    }
}
