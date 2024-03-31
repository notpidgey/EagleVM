#include "eaglevm-core/virtual_machine/il/il_bb.h"

namespace eagle::il
{
    void il_bb::add_command(const base_command_ptr& command)
    {
        commands.push_back(command);
    }

    bool il_bb::insert_after(const base_command_ptr& command_ptr)
    {
        const auto it = get_iterator(command_ptr);
        if (it == commands.end())
            return false;

        commands.insert(it, command_ptr);
        return true;
    }

    bool il_bb::insert_before(const base_command_ptr& command_ptr)
    {
        const auto it = get_iterator(command_ptr);
        if (it == commands.end())
            return false;

        commands.insert(it, command_ptr);
        return true;
    }

    std::vector<base_command_ptr>::iterator il_bb::get_iterator(base_command_ptr command)
    {
        return std::ranges::find_if(commands,
            [&command](const base_command_ptr& command_ptr)
            {
                return command_ptr == command;
            });
    }
}
