#pragma once
#include <array>
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    template <command_type T, int param_count = 2>
    class cmd_arith_base final : public base_command
    {
    public:
        explicit cmd_arith_base(const ir_size size, const bool reversed = false, const bool preserve_args = false)
            : base_command(T), size(size), reversed(reversed), preserved(preserve_args)
        {
        }

        uint8_t get_param_count() { return param_count; }

        ir_size get_size() const { return size; }
        bool get_reversed() const { return reversed; }
        bool get_preserved() const { return preserved; }

        bool is_similar(const std::shared_ptr<base_command>& other) override
        {
            std::shared_ptr<cmd_arith_base> cmd = std::static_pointer_cast<cmd_arith_base>(other);
            return base_command::is_similar(other) && cmd->get_preserved() == preserved && cmd->get_param_count() == get_param_count() && cmd->
                get_size() == size && cmd->get_reversed() == reversed;
        }

        std::string to_string() override
        {
            return base_command::to_string() + " (" + ir_size_to_string(size) + ", " + ir_size_to_string(size) + ")";
        }

        BASE_COMMAND_CLONE(cmd_arith_base);

    private:
        ir_size size;
        bool reversed;
        bool preserved;
    };

    template <command_type T>
    class cmd_arith_base<T, 1> final : public base_command
    {
    public:
        explicit cmd_arith_base(const ir_size size, const bool preserve_args = false)
            : base_command(T), size(size), preserved(preserve_args)
        {
        }

        uint8_t get_param_count() { return 1; }

        ir_size get_size() const { return size; }
        bool get_preserved() const { return preserved; }

        bool is_similar(const std::shared_ptr<base_command>& other) override
        {
            std::shared_ptr<cmd_arith_base> cmd = std::static_pointer_cast<cmd_arith_base>(other);
            return cmd->get_preserved() == preserved && cmd->get_param_count() == get_param_count() && cmd->get_size() == size;
        }

        BASE_COMMAND_CLONE(cmd_arith_base);

    private:
        ir_size size;
        bool preserved;
    };

    // bitwise
    using cmd_and = cmd_arith_base<command_type::vm_and>;
    using cmd_or = cmd_arith_base<command_type::vm_or>;
    using cmd_xor = cmd_arith_base<command_type::vm_xor>;
    using cmd_shl = cmd_arith_base<command_type::vm_shl>;
    using cmd_shr = cmd_arith_base<command_type::vm_shr>;
    using cmd_cnt = cmd_arith_base<command_type::vm_cnt>;

    // arith
    using cmd_add = cmd_arith_base<command_type::vm_add>;
    using cmd_sub = cmd_arith_base<command_type::vm_sub>;
    using cmd_smul = cmd_arith_base<command_type::vm_smul>;
    using cmd_umul = cmd_arith_base<command_type::vm_umul>;

    using cmd_abs = cmd_arith_base<command_type::vm_abs, 1>;
    using cmd_log2 = cmd_arith_base<command_type::vm_log2, 1>;

    // other
    using cmd_dup = cmd_arith_base<command_type::vm_dup, 1>;

    SHARED_DEFINE(cmd_and);
    SHARED_DEFINE(cmd_or);
    SHARED_DEFINE(cmd_xor);
    SHARED_DEFINE(cmd_shl);
    SHARED_DEFINE(cmd_shr);
    SHARED_DEFINE(cmd_cnt);

    SHARED_DEFINE(cmd_add);
    SHARED_DEFINE(cmd_sub);
    SHARED_DEFINE(cmd_smul);
    SHARED_DEFINE(cmd_umul);

    SHARED_DEFINE(cmd_abs);
    SHARED_DEFINE(cmd_log2);

    SHARED_DEFINE(cmd_dup);
}
