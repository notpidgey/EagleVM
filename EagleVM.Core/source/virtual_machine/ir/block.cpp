#include "eaglevm-core/virtual_machine/ir/block.h"
#include "eaglevm-core/virtual_machine/ir/commands/include.h"

namespace eagle::il
{
    base_command_ptr block_il::add_command(const base_command_ptr& command)
    {
        commands.push_back(command);
        return command;
    }

    void block_il::copy_from(const block_il_ptr& other)
    {
        commands.append_range(other->commands);
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

    std::vector<base_command_ptr>::iterator block_il::get_iterator(base_command_ptr command)
    {
        return std::ranges::find_if(commands,
            [&command](const base_command_ptr& command_ptr)
            {
                return command_ptr == command;
            });
    }

    void block_il::set_exit(const cmd_exit_ptr& exit_result)
    {
        if(exit != nullptr)
            commands.pop_back();

        exit = exit_result;
        commands.push_back(exit_result);
    }
}
