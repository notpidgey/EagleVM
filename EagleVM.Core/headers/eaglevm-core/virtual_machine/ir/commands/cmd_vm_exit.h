#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/branch_command.h"

namespace eagle::ir
{
    class cmd_vm_exit : public branch_command, public base_command
    {
    public:
        explicit cmd_vm_exit(const ir_exit_result& result);

        bool is_similar(const std::shared_ptr<base_command>& other) override;
        ir_exit_result get_exit();
        std::string to_string() override;

        BASE_COMMAND_CLONE(cmd_vm_exit);
    };

    SHARED_DEFINE(cmd_vm_exit);
}
