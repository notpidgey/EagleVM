#pragma once
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_branch.h"
#include "eaglevm-core/codec/zydis_helper.h"

namespace eagle::ir
{
    class block_ir;
    using block_il_ptr = std::shared_ptr<block_ir>;

    class block_ir
    {
    public:
        explicit block_ir(const bool flag_aware = false)
            : exit(nullptr), flag_aware(flag_aware)
        {
        }

        base_command_ptr add_command(const base_command_ptr& command);
        void add_command(const std::vector<base_command_ptr>& command);

        void copy_from(const block_il_ptr& other);
        bool insert_after(const base_command_ptr& command_ptr);
        bool insert_before(const base_command_ptr& command_ptr);

        base_command_ptr get_command(size_t i);
        base_command_ptr get_command_back();
        size_t get_command_count() const;

    private:
        std::vector<base_command_ptr> commands;
        cmd_branch_ptr exit;

        bool flag_aware = false;

        std::vector<base_command_ptr>::iterator get_iterator(base_command_ptr command);
    };
}

