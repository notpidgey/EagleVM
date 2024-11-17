#pragma once
#include "eaglevm-core/virtual_machine/ir/x86/models/flags.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_stack.h"

namespace eagle::ir
{
    class cmd_context_rflags_store : public base_command
    {
    public:
        explicit cmd_context_rflags_store(const x86_cpu_flag relevant_flags)
            : base_command(command_type::vm_context_rflags_store), relevant_flags(relevant_flags)
        {
        }

        x86_cpu_flag get_relevant_flags() const
        {
            return relevant_flags;
        }

        bool is_similar(const base_command_ptr& other) override
        {
            const auto cmd = std::static_pointer_cast<cmd_context_rflags_store>(other);
            return get_relevant_flags() == cmd->get_relevant_flags();
        }

        std::string to_string() override
        {
            return base_command::to_string() + " " + x86_cpu_flag_to_string(relevant_flags);
        };

        BASE_COMMAND_CLONE(cmd_context_rflags_store);

    private:
        x86_cpu_flag relevant_flags;
    };

    SHARED_DEFINE(cmd_context_rflags_store);
}
