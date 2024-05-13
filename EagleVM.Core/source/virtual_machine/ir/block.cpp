#include "eaglevm-core/virtual_machine/ir/block.h"
#include "eaglevm-core/virtual_machine/ir/commands/include.h"

namespace eagle::il
{
    base_command_ptr block_il::add_command(const base_command_ptr& command)
    {
        if(command->get_command_type() == command_type::vm_branch)
        {
            assert(exit == nullptr, "cannot have two exiting commands");
            exit = std::static_pointer_cast<cmd_branch>(command);
        }

        commands.push_back(command);
        return command;
    }

    void block_il::copy_from(const block_il_ptr& other)
    {
        commands.append_range(other->commands);

        flag_aware = other->flag_aware;
        exit = other->exit;
    }

    bool block_il::insert_after(const base_command_ptr& command_ptr)
    {
        const auto it = get_iterator(command_ptr);
        if (it == commands.end())
            return false;

        commands.insert(it, command_ptr);
        return true;
    }

    bool block_il::insert_before(const base_command_ptr& command_ptr)
    {
        const auto it = get_iterator(command_ptr);
        if (it == commands.end())
            return false;

        commands.insert(it, command_ptr);
        return true;
    }

    base_command_ptr block_il::get_command(const size_t i)
    {
        assert(i < commands.size(), "index beyond vector size");
        return commands[i];
    }

    std::vector<base_command_ptr>::iterator block_il::get_iterator(base_command_ptr command)
    {
        return std::ranges::find_if(commands,
            [&command](const base_command_ptr& command_ptr)
            {
                return command_ptr == command;
            });
    }

    size_t block_il::get_command_count() const
    {
        return commands.size();
    }
}
