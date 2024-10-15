#pragma once
#include <array>
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    template <command_type T>
    class cmd_base : public base_command
    {
    public:
        cmd_base(ir_size size) : base_command(T), size(size) {}

        ir_size get_size() const { return size; }

    private:
        ir_size size;
    };

    using cmd_and = cmd_base<command_type::vm_and>;
    using cmd_or = cmd_base<command_type::vm_or>;
    using cmd_xor = cmd_base<command_type::vm_xor>;
    using cmd_shl = cmd_base<command_type::vm_shl>;
    using cmd_shr = cmd_base<command_type::vm_shr>;
}
