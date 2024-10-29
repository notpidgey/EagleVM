#pragma once
#include <utility>
#include <vector>
#include <memory>
#include <algorithm>
#include <iterator>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
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

        explicit block_ir(const block_state state)
            : exit_(nullptr), block_state_(state)
        {
            static uint32_t current_block_id = 0;
            block_id = current_block_id++;
        }

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
            exit_ = nullptr;
        }

        iterator insert(const const_iterator& pos, const base_command_ptr& command)
        {
            auto it = commands_.insert(pos, command);
            update_exit(command);

            return it;
        }

        iterator erase(const const_iterator& pos)
        {
            auto it = commands_.erase(pos);
            update_exit();

            return it;
        }

        void push_back(const base_command_ptr& command)
        {
            commands_.push_back(command);
            update_exit(command);
        }

        void push_back(const std::vector<base_command_ptr>& command)
        {
            commands_.append_range(command);
            update_exit(command.back());
        }

        void pop_back()
        {
            if (!commands_.empty())
            {
                commands_.pop_back();
                update_exit();
            }
        }

        void copy_from(const block_ptr& other)
        {
            VM_ASSERT(other->get_block_state() == block_state_, "block states must match");
            commands_.insert(commands_.end(), other->begin(), other->end());
        };

        cmd_branch_ptr& get_branch() { return exit_; }
        block_state get_block_state() const { return block_state_; }
        void set_block_state(const block_state state) { block_state_ = state; }

        template <typename T>
        std::shared_ptr<T> get_command(const size_t i, const command_type command_assert = command_type::none) const
        {
            const auto& command = at(i);
            VM_ASSERT(command_assert != command_type::none &&
                command->get_command_type() == command_assert, "command assert failed, invalid command type");

            return std::static_pointer_cast<T>(command);
        }

    private:
        container_type commands_;
        cmd_branch_ptr exit_;
        block_state block_state_;

        void update_exit(const base_command_ptr& command = nullptr)
        {
            if (command && command->get_command_type() == command_type::vm_branch)
                exit_ = std::static_pointer_cast<cmd_branch>(command);
            else
            {
                // if the parameter is null, it means we removed a command from the list
                if (commands_.empty()) // we ran out of commands, so exit has to be null
                    exit_ = nullptr;
                else if (commands_.back()->get_command_type() == command_type::vm_branch)
                    exit_ = std::static_pointer_cast<cmd_branch>(commands_.back());
            }
        }
    };


}
