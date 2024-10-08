#include "eaglevm-core/virtual_machine/ir/block.h"
#include "eaglevm-core/virtual_machine/ir/commands/include.h"

namespace eagle::ir
{
    base_command_ptr block_ir::add_command(const base_command_ptr& command)
    {
        VM_ASSERT(exit == nullptr, "cannot append command after exit");
        if (command->get_command_type() == command_type::vm_branch)
            exit = std::static_pointer_cast<cmd_branch>(command);

        commands.push_back(command);
        return command;
    }

    void block_ir::add_command(const std::vector<base_command_ptr>& command)
    {
        VM_ASSERT(exit == nullptr, "cannot append command after exit");
        if (!command.empty() && command.back()->get_command_type() == command_type::vm_branch)
            exit = std::static_pointer_cast<cmd_branch>(command.back());

        commands.append_range(command);
    }

    void block_ir::copy_from(const block_ptr& other)
    {
        commands.append_range(other->commands);

        flag_aware = other->flag_aware;
        exit = other->exit;
    }

    bool block_ir::insert_after(const base_command_ptr& command_ptr)
    {
        const auto it = get_iterator(command_ptr);
        if (it == commands.end())
            return false;

        commands.insert(it, command_ptr);
        return true;
    }

    bool block_ir::insert_before(const base_command_ptr& command_ptr)
    {
        const auto it = get_iterator(command_ptr);
        if (it == commands.end())
            return false;

        commands.insert(it, command_ptr);
        return true;
    }

    base_command_ptr& block_ir::get_command(const size_t i)
    {
        VM_ASSERT(i < commands.size(), "index beyond vector size");
        return commands[i];
    }

    base_command_ptr& block_ir::get_command_back()
    {
        VM_ASSERT(!commands.empty(), "obfuscation cannot be empty");
        return commands.back();
    }

    std::vector<base_command_ptr>::iterator block_ir::get_iterator(base_command_ptr command)
    {
        return std::ranges::find_if(commands,
            [&command](const base_command_ptr& command_ptr)
            {
                return command_ptr == command;
            });
    }

    size_t block_ir::get_command_count() const
    {
        return commands.size();
    }

    base_command_ptr& block_ir::remove_command(const size_t i)
    {
        VM_ASSERT(i < commands.size(), "index beyond vector size");

        base_command_ptr& command = commands[i];
        commands.erase(commands.begin() + i);

        return command;
    }

    cmd_branch_ptr& block_ir::get_branch()
    {
        return exit;
    }
}
