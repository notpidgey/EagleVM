#pragma once
#include <array>
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    /*
     * sets vm_flags::eq, vm_flags::le, le::ge
     * flag register is updated
     */
    class cmd_cmp final : public base_command
    {
    public:
        explicit cmd_cmp(const ir_size size) : base_command(command_type::vm_cmp), size(size) {}
        ir_size get_size() const { return size; }

        BASE_COMMAND_CLONE(cmd_cmp);

    private:
        ir_size size;
    };

    SHARED_DEFINE(cmd_cmp);
}
