#pragma once
#include <array>
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    template <command_type T>
    class cmd_arith_base final : public base_command
    {
    public:
        explicit cmd_arith_base(const ir_size size) : base_command(T), size(size) {}
        ir_size get_size() const { return size; }

        bool is_similar(const std::shared_ptr<base_command>& other) override { return true; }

    private:
        ir_size size;
    };

    // bitwise
    using cmd_and = cmd_arith_base<command_type::vm_and>;
    using cmd_or  = cmd_arith_base<command_type::vm_or>;
    using cmd_xor = cmd_arith_base<command_type::vm_xor>;
    using cmd_shl = cmd_arith_base<command_type::vm_shl>;
    using cmd_shr = cmd_arith_base<command_type::vm_shr>;

    // arith
    using cmd_add = cmd_arith_base<command_type::vm_add>;
    using cmd_sub = cmd_arith_base<command_type::vm_sub>;

    SHARED_DEFINE(cmd_and);
    SHARED_DEFINE(cmd_or);
    SHARED_DEFINE(cmd_xor);
    SHARED_DEFINE(cmd_shl);
    SHARED_DEFINE(cmd_shr);
    SHARED_DEFINE(cmd_add);
    SHARED_DEFINE(cmd_sub);
}
