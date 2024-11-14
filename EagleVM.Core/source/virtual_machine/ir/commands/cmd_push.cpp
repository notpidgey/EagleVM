#include "eaglevm-core/virtual_machine/ir/commands/cmd_push.h"

namespace eagle::ir
{
    cmd_push::cmd_push(const push_v& reg_src, const ir_size reg_size)
        : base_command(command_type::vm_push), size(reg_size), value(reg_src)
    {
    }

    cmd_push::cmd_push(uint64_t immediate, const ir_size stack_disp)
        : base_command(command_type::vm_push), size(stack_disp), value(immediate)
    {
    }

    ir_size cmd_push::get_size() const
    {
        return size;
    }

    push_v cmd_push::get_value() const
    {
        return value;
    }

    template<class... Ts>
    struct overloaded : Ts... { using Ts::operator()...; };

    bool cmd_push::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_push>(other);
        return base_command::is_similar(other) &&
            get_size() == cmd->get_size() &&
            std::visit(overloaded{
                [](const uint64_t a, const uint64_t b) { return a == b; },
                [](const reg_vm a, const reg_vm b) { return a == b; },
                [](const block_ptr& a, const block_ptr& b) { return a == b; },
                [](auto a, auto b) { return false; },
            }, value, cmd->value);
    }

    std::string cmd_push::to_string()
    {
        auto resolve_push = [&]
        {
            return std::visit([](auto&& arg) -> std::string
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, uint64_t>)
                {
                    const uint64_t immediate_value = arg;
                    return std::format("0x{:x}", immediate_value);
                }
                else if constexpr (std::is_same_v<T, block_ptr>)
                {
                    const block_ptr block = arg;
                    return "block(" + std::to_string(block->block_id) + ")";
                }
                else if constexpr (std::is_same_v<T, discrete_store_ptr>)
                {
                    return "(store)";
                }
                else if constexpr (std::is_same_v<T, reg_vm>)
                {
                    const reg_vm reg = arg;
                    return reg_vm_to_string(reg);
                }
                else
                VM_ASSERT("unknown push type handled");

                return "";
            }, value);
        };

        return base_command::to_string() + "(" + ir_size_to_string(size) + ") " + resolve_push();
    }
}
