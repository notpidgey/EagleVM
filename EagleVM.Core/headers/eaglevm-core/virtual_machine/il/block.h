#pragma once
#include <vector>
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"
#include "eaglevm-core/virtual_machine/il/commands/cmd_exit.h"
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
        bool insert_after(const base_command_ptr& command_ptr);
        bool insert_before(const base_command_ptr& command_ptr);
        void set_exit(const cmd_exit_ptr& exit_result);

    private:
        std::vector<base_command_ptr> commands;
        cmd_exit_ptr exit;

        uint8_t exit_nodes_count = 0;
        bool flag_aware = false;

        std::vector<base_command_ptr>::iterator get_iterator(base_command_ptr command);
    };

}
