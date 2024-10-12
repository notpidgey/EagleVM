#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_stack.h"

namespace eagle::ir
{
    class cmd_rflags_store : public base_command
    {
    public:
        explicit cmd_rflags_store(const std::vector<uint8_t>& flag_indexes)
            : base_command(command_type::vm_rflags_store), relevant_flags(flag_indexes)
        {
        }

        std::vector<uint8_t> get_relevant_flags() const
        {
            return relevant_flags;
        }

    private:
        std::vector<uint8_t> relevant_flags;
    };
}
