#pragma once
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_branch.h"
#include "eaglevm-core/codec/zydis_helper.h"

namespace eagle::ir
{
    class block_ir;
    using block_ptr = std::shared_ptr<block_ir>;

    class block_ir
    {
    public:
        explicit block_ir(const bool flag_aware = false)
            : exit(nullptr), flag_aware(flag_aware)
        {
        }

        base_command_ptr add_command(const base_command_ptr& command);
        void add_command(const std::vector<base_command_ptr>& command);

        void copy_from(const block_ptr& other);
        bool insert_after(const base_command_ptr& command_ptr);
        bool insert_before(const base_command_ptr& command_ptr);

        base_command_ptr& get_command(size_t i);

        template <typename T>
        std::shared_ptr<T>& get_command(const size_t i, const command_type command_assert = command_type::none)
        {
            const base_command_ptr& command = get_command(i);
            if (command_assert != command_type::none)
                VM_ASSERT(command->get_command_type() == command_assert, "command assert failed, invalid command type");

            return std::static_pointer_cast<T>(command);
        }

        base_command_ptr& get_command_back();
        size_t get_command_count() const;

        base_command_ptr& remove_command(size_t i);

        cmd_branch_ptr& get_branch();

    private:
        std::vector<base_command_ptr> commands;
        cmd_branch_ptr exit;

        bool flag_aware = false;

        std::vector<base_command_ptr>::iterator get_iterator(base_command_ptr command);
    };
}
