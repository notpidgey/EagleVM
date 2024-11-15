#pragma once
#include <utility>
#include <vector>
#include <memory>
#include <algorithm>
#include <iterator>

#include "commands/cmd_cf.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_branch.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_vm_exit.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_branch.h"
#include "eaglevm-core/codec/zydis_helper.h"
#include "models/ir_block_action.h"

namespace eagle::ir
{
    class block_ir;
    using block_ptr = std::shared_ptr<block_ir>;

    class block_ir : public std::enable_shared_from_this<block_ir>
    {
    public:
        using container_type = std::vector<base_command_ptr>;
        using iterator = container_type::iterator;
        using const_iterator = container_type::const_iterator;
        using reverse_iterator = container_type::reverse_iterator;
        using const_reverse_iterator = container_type::const_reverse_iterator;

        uint32_t block_id;

        iterator begin() { return commands_.begin(); }
        const_iterator begin() const { return commands_.begin(); }
        iterator end() { return commands_.end(); }
        const_iterator end() const { return commands_.end(); }
        reverse_iterator rbegin() { return commands_.rbegin(); }
        const_reverse_iterator rbegin() const { return commands_.rbegin(); }
        reverse_iterator rend() { return commands_.rend(); }
        const_reverse_iterator rend() const { return commands_.rend(); }

        size_t size() const { return commands_.size(); }
        bool empty() const { return commands_.empty(); }

        base_command_ptr& front() { return commands_.front(); }
        const base_command_ptr& front() const { return commands_.front(); }
        base_command_ptr& back() { return commands_.back(); }
        const base_command_ptr& back() const { return commands_.back(); }
        base_command_ptr& at(const size_t pos) { return commands_.at(pos); }
        const base_command_ptr& at(const size_t pos) const { return commands_.at(pos); }
        base_command_ptr& operator[](const size_t pos) { return commands_[pos]; }
        const base_command_ptr& operator[](const size_t pos) const { return commands_[pos]; }

        void clear()
        {
            commands_.clear();
            exit_cmd = nullptr;
        }

        iterator insert(const const_iterator& pos, const base_command_ptr& command)
        {
            auto it = commands_.insert(pos, command);
            handle_cmd_insert(command);

            return it;
        }

        iterator erase(const const_iterator& pos)
        {
            auto it = commands_.erase(pos);
            handle_cmd_insert();

            return it;
        }

        iterator erase(const const_iterator& pos, const const_iterator& pos2)
        {
            auto it = commands_.erase(pos, pos2);
            handle_cmd_insert();

            return it;
        }

        void push_back(const base_command_ptr& command)
        {
            commands_.push_back(command);
            handle_cmd_insert(command);
        }

        void push_back(const std::vector<base_command_ptr>& command)
        {
            commands_.append_range(command);
            handle_cmd_insert(command.back());
        }

        void pop_back()
        {
            if (!commands_.empty())
            {
                commands_.pop_back();
                handle_cmd_insert();
            }
        }

        void copy_from(const block_ptr& other)
        {
            VM_ASSERT(other->get_block_state() == ir_state, "block states must match");
            commands_.insert(commands_.end(), other->begin(), other->end());
        };

        block_state get_block_state() const { return ir_state; }

        template <typename T>
        std::shared_ptr<T> get_command(const size_t i, const command_type command_assert = command_type::none) const
        {
            const auto& command = at(i);
            VM_ASSERT(command_assert != command_type::none &&
                command->get_command_type() == command_assert, "command assert failed, invalid command type");

            return std::static_pointer_cast<T>(command);
        }

        std::shared_ptr<class block_virt_ir> as_virt()
        {
            if (ir_state == vm_block) return std::static_pointer_cast<class block_virt_ir>(shared_from_this());
            return nullptr;
        };

        std::shared_ptr<class block_x86_ir> as_x86()
        {
            if (ir_state == x86_block) return std::static_pointer_cast<class block_x86_ir>(shared_from_this());
            return nullptr;
        };

    protected:
        ~block_ir() = default;

        explicit block_ir(const block_state state, const uint32_t custom_block_id = -1)
            : exit_cmd(nullptr), ir_state(state)
        {
            static uint32_t current_block_id = 0;
            if (custom_block_id == -1)
                block_id = current_block_id++;
            else
                block_id = custom_block_id;
        }

        container_type commands_;
        base_command_ptr exit_cmd;
        block_state ir_state;

        virtual void handle_cmd_insert(const base_command_ptr& command = nullptr) = 0;
    };

    class block_virt_ir final : public block_ir
    {
    public:
        explicit block_virt_ir(const uint32_t custom_block_id = -1)
            : block_ir(vm_block, custom_block_id)
        {
        }

        cmd_vm_exit_ptr exit_as_vmexit() const
        {
            if (exit_cmd && exit_cmd->get_command_type() == command_type::vm_exit)
                return std::static_pointer_cast<cmd_vm_exit>(exit_cmd);

            return nullptr;
        }

        cmd_branch_ptr exit_as_branch() const
        {
            if (exit_cmd && exit_cmd->get_command_type() == command_type::vm_branch)
                return std::static_pointer_cast<cmd_branch>(exit_cmd);

            return nullptr;
        }

        std::vector<block_ptr> get_calls() const
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

    private:
        void handle_cmd_insert(const base_command_ptr& command) override
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
    };

    class block_x86_ir final : public block_ir
    {
    public:
        explicit block_x86_ir(const uint32_t custom_block_id = -1)
            : block_ir(x86_block, custom_block_id)
        {
        }

        cmd_branch_ptr exit_as_branch() const
        {
            if (exit_cmd && exit_cmd->get_command_type() == command_type::vm_branch)
                return std::static_pointer_cast<cmd_branch>(exit_cmd);

            return nullptr;
        }

    private:
        void handle_cmd_insert(const base_command_ptr& command) override
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
    };

    using block_x86_ir_ptr = std::shared_ptr<class block_x86_ir>;
    using block_virt_ir_ptr = std::shared_ptr<class block_virt_ir>;
}
