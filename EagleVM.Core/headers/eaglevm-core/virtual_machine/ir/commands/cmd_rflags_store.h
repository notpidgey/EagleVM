#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_stack.h"

namespace eagle::ir
{
    class cmd_rflags_store : public base_command
    {
    public:
        explicit cmd_rflags_store(const uint64_t flag_mask)
            : base_command(command_type::vm_rflags_store), relevant_flags(flag_mask)
        {
        }

        uint64_t get_relevant_flags() const
        {
            return relevant_flags;
        }

    private:
        uint64_t relevant_flags;
    };
}
