#pragma once
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_exit.h"
#include "eaglevm-core/codec/zydis_helper.h"

namespace eagle::il
{
    class block_il;
    using block_il_ptr = std::shared_ptr<block_il>;

    class block_il
    {
    public:
        explicit block_il(const bool flag_aware = false)
            : exit(nullptr), flag_aware(flag_aware)
        {
        }

        base_command_ptr add_command(const base_command_ptr& command);
        void copy_from(const block_il_ptr& other);
        bool insert_after(const base_command_ptr& command_ptr);
        bool insert_before(const base_command_ptr& command_ptr);

        base_command_ptr get_command(size_t i);
        size_t get_command_count() const;

    private:
        std::vector<base_command_ptr> commands;
        cmd_exit_ptr exit;

        bool flag_aware = false;

        std::vector<base_command_ptr>::iterator get_iterator(base_command_ptr command);
    };
}

