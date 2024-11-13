#pragma once
#include <array>
#include <vector>

#include "eaglevm-core/virtual_machine/ir/block.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
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
