#pragma once
#include <array>
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    using block_ptr = std::shared_ptr<class block_ir>;
    class cmd_call final : public base_command
    {
    public:
        explicit cmd_call(const block_ptr& target)
            : base_command(command_type::vm_call), target(target)
        {
        }

        block_ptr get_target()
        {
            return target;
        }

        bool is_similar(const std::shared_ptr<base_command>& other) override
        {
            const auto cmd = std::static_pointer_cast<cmd_call>(other);
            return base_command::is_similar(other) && target == cmd->target;
        }

        std::string to_string() override;

        BASE_COMMAND_CLONE(cmd_call);

    private:
        block_ptr target;
    };

    class cmd_ret final : public base_command
    {
    public:
        explicit cmd_ret()
            : base_command(command_type::vm_ret)
        {
        }

        BASE_COMMAND_CLONE(cmd_ret);
    };

    SHARED_DEFINE(cmd_call);
    SHARED_DEFINE(cmd_ret);
}
