#include "eaglevm-core/virtual_machine/ir/block.h"

namespace eagle::ir
{
    block_ir::iterator block_ir::begin()
    {
        return commands_.begin();
    }

    block_ir::const_iterator block_ir::begin() const
    {
        return commands_.begin();
    }

    block_ir::iterator block_ir::end()
    {
        return commands_.end();
    }

    block_ir::const_iterator block_ir::end() const
    {
        return commands_.end();
    }

    block_ir::reverse_iterator block_ir::rbegin()
    {
        return commands_.rbegin();
    }

    block_ir::const_reverse_iterator block_ir::rbegin() const
    {
        return commands_.rbegin();
    }

    block_ir::reverse_iterator block_ir::rend()
    {
        return commands_.rend();
    }

    block_ir::const_reverse_iterator block_ir::rend() const
    {
        return commands_.rend();
    }

    size_t block_ir::size() const
    {
        return commands_.size();
    }

    bool block_ir::empty() const
    {
        return commands_.empty();
    }

    base_command_ptr& block_ir::front()
    {
        return commands_.front();
    }

    const base_command_ptr& block_ir::front() const
    {
        return commands_.front();
    }

    base_command_ptr& block_ir::back()
    {
        return commands_.back();
    }

    const base_command_ptr& block_ir::back() const
    {
        return commands_.back();
    }

    base_command_ptr& block_ir::at(const size_t pos)
    {
        return commands_.at(pos);
    }

    const base_command_ptr& block_ir::at(const size_t pos) const
    {
        return commands_.at(pos);
    }

    base_command_ptr& block_ir::operator[](const size_t pos)
    {
        return commands_[pos];
    }

    const base_command_ptr& block_ir::operator[](const size_t pos) const
    {
        return commands_[pos];
    }

    void block_ir::clear()
    {
        commands_.clear();
        exit_cmd = nullptr;
    }

    block_ir::iterator block_ir::insert(const const_iterator& pos, const base_command_ptr& command)
    {
        auto it = commands_.insert(pos, command);
        handle_cmd_insert(command);

        return it;
    }

    block_ir::iterator block_ir::erase(const const_iterator& pos)
    {
        auto it = commands_.erase(pos);
        handle_cmd_insert();

        return it;
    }

    block_ir::iterator block_ir::erase(const const_iterator& pos, const const_iterator& pos2)
    {
        auto it = commands_.erase(pos, pos2);
        handle_cmd_insert();

        return it;
    }

    void block_ir::push_back(const base_command_ptr& command)
    {
        commands_.push_back(command);
        handle_cmd_insert(command);
    }

    void block_ir::push_back(const std::vector<base_command_ptr>& command)
    {
        commands_.append_range(command);
        handle_cmd_insert(command.back());
    }

    void block_ir::pop_back()
    {
        if (!commands_.empty())
        {
            commands_.pop_back();
            handle_cmd_insert();
        }
    }

    void block_ir::copy_from(const block_ptr& other)
    {
        VM_ASSERT(other->get_block_state() == ir_state, "block states must match");
        commands_.insert(commands_.end(), other->begin(), other->end());
    }

    block_state block_ir::get_block_state() const
    {
        return ir_state;
    }

    template <typename T>
    std::shared_ptr<T> block_ir::get_command(const size_t i, const command_type command_assert) const
    {
        const auto& command = at(i);
        if (command_assert != command_type::none)
        {
            VM_ASSERT(command->get_command_type() == command_assert, "command assert failed, invalid command type");
        }

        return std::static_pointer_cast<T>(command);
    }

    std::shared_ptr<class block_virt_ir> block_ir::as_virt()
    {
        if (ir_state == vm_block) return std::static_pointer_cast<class block_virt_ir>(shared_from_this());
        return nullptr;
    }

    std::shared_ptr<class block_x86_ir> block_ir::as_x86()
    {
        if (ir_state == x86_block) return std::static_pointer_cast<class block_x86_ir>(shared_from_this());
        return nullptr;
    }

    block_ir::~block_ir() = default;

    block_ir::block_ir(const block_state state, const uint32_t custom_block_id): exit_cmd(nullptr), ir_state(state)
    {
        static uint32_t current_block_id = 0;
        if (custom_block_id == UINT32_MAX)
            block_id = current_block_id++;
        else
            block_id = custom_block_id;
    }

    block_virt_ir::block_virt_ir(const uint32_t custom_block_id): block_ir(vm_block, custom_block_id)
    {
    }

    cmd_vm_exit_ptr block_virt_ir::exit_as_vmexit() const
    {
        if (exit_cmd && exit_cmd->get_command_type() == command_type::vm_exit)
            return std::static_pointer_cast<cmd_vm_exit>(exit_cmd);

        return nullptr;
    }

    cmd_branch_ptr block_virt_ir::exit_as_branch() const
    {
        if (exit_cmd && exit_cmd->get_command_type() == command_type::vm_branch)
            return std::static_pointer_cast<cmd_branch>(exit_cmd);

        return nullptr;
    }

    std::vector<block_ptr> block_virt_ir::get_calls() const
    {
        std::vector<block_ptr> calls;
        for (auto cmd : commands_)
        {
            if (cmd->get_command_type() == command_type::vm_call)
            {
                const auto call = std::static_pointer_cast<cmd_call>(cmd);
                calls.push_back(call->get_target());
            }
        }

        return calls;
    }

    void block_virt_ir::handle_cmd_insert(const base_command_ptr& command)
    {
        if (commands_.empty())
        {
            exit_cmd = nullptr;
            return;
        }

        const auto& last_command = commands_.back();
        const auto cmd_type = last_command->get_command_type();

        if (cmd_type == command_type::vm_branch || cmd_type == command_type::vm_exit || cmd_type == command_type::vm_ret)
        {
            // Update exit_cmd to the last command
            exit_cmd = last_command;

            // Additional validation
            if (const auto branch = exit_as_branch())
                VM_ASSERT(branch->is_virtual(), "branch from virtual block must be virtual");
        }
        else
        {
            exit_cmd = nullptr;
        }
    }

    block_x86_ir::block_x86_ir(const uint32_t custom_block_id): block_ir(x86_block, custom_block_id)
    {
    }

    cmd_branch_ptr block_x86_ir::exit_as_branch() const
    {
        if (exit_cmd && exit_cmd->get_command_type() == command_type::vm_branch)
            return std::static_pointer_cast<cmd_branch>(exit_cmd);

        return nullptr;
    }

    void block_x86_ir::handle_cmd_insert(const base_command_ptr& command)
    {
        if (commands_.empty())
        {
            exit_cmd = nullptr;
            return;
        }

        const auto& last_command = commands_.back();
        const auto cmd_type = last_command->get_command_type();

        if (cmd_type == command_type::vm_branch)
        {
            // Update exit_cmd to the last command
            exit_cmd = last_command;

            // Additional validation
            VM_ASSERT(exit_as_branch()->is_virtual() == false, "branch from x86 block must not be virtual");
        }
        else
        {
            exit_cmd = nullptr;
        }
    }
}
