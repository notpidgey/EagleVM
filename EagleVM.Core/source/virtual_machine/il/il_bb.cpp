#include "eaglevm-core/virtual_machine/il/il_bb.h"
#include "eaglevm-core/virtual_machine/il/commands/include.h"

namespace eagle::il
{
    base_command_ptr il_bb::add_command(const base_command_ptr& command)
    {
        commands.push_back(command);
        return command;
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

    bool il_bb::get_inline() const
    {
        return inline_next;
    }

    void il_bb::set_exit(const cmd_exit_ptr& exit)
    {
        exit_command = exit;
        add_command(std::dynamic_pointer_cast<base_command>(exit));
    }

    void il_bb::set_inline(const bool inline_exit_block)
    {
        inline_next = inline_exit_block;
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
