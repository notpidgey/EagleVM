#pragma once
#include <utility>
#include <vector>
#include <memory>
#include <algorithm>
#include <iterator>

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

    class block_ir
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
            if (exit_cmd->get_command_type() == command_type::vm_exit)
                return std::static_pointer_cast<cmd_vm_exit>(exit_cmd);

            return nullptr;
        }

        cmd_branch_ptr exit_as_branch() const
        {
            if (exit_cmd->get_command_type() == command_type::vm_branch)
                return std::static_pointer_cast<cmd_branch>(exit_cmd);

            return nullptr;
        }

    private:
        void handle_cmd_insert(const base_command_ptr& command) override
        {
            if (command)
            {
                const auto command_type = command->get_command_type();
                if (command_type == command_type::vm_branch || command_type == command_type::vm_exit)
                {
                    VM_ASSERT(exit_cmd == nullptr, "unable to have 2 exiting instructions at the end of block");

                    exit_cmd = command;
                    if (const auto branch = exit_as_branch())
                    {
                        VM_ASSERT(branch->is_virtual(), "branch from virtual block must be virtual");
                    }
                }
                else
                {
                    VM_ASSERT(command_type != command_type::vm_exec_dynamic_x86 && command_type != command_type::vm_exec_x86,
                        "vm block may not execute x86 instructions");
                }
            }
            else exit_cmd = nullptr;
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
            if (exit_cmd->get_command_type() == command_type::vm_branch)
                return std::static_pointer_cast<cmd_branch>(exit_cmd);

            return nullptr;
        }

    private:
        void handle_cmd_insert(const base_command_ptr& command) override
        {
            if (command)
            {
                const auto command_type = command->get_command_type();
                if (command_type == command_type::vm_branch)
                {
                    VM_ASSERT(exit_cmd == nullptr, "unable to have 2 exiting instructions at the end of block");

                    exit_cmd = command;
                    VM_ASSERT(exit_as_branch()->is_virtual() == false, "branch from x86 block must not be virtual");
                }
                else
                {
                    VM_ASSERT(command_type == command_type::vm_exec_dynamic_x86 || command_type == command_type::vm_exec_x86,
                        "x86 block may only execute x86 instructions");
                }
            }
            else exit_cmd = nullptr;
        }
    };

    using block_x86_ir_ptr = std::shared_ptr<class block_x86_ir>;
    using block_virt_ir_ptr = std::shared_ptr<class block_virt_ir>;
}
