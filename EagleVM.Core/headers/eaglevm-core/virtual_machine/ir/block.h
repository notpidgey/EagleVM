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

        iterator begin();
        const_iterator begin() const;
        iterator end();
        const_iterator end() const;
        reverse_iterator rbegin();
        const_reverse_iterator rbegin() const;
        reverse_iterator rend();
        const_reverse_iterator rend() const;

        size_t size() const;
        bool empty() const;

        base_command_ptr& front();
        const base_command_ptr& front() const;
        base_command_ptr& back();
        const base_command_ptr& back() const;
        base_command_ptr& at(const size_t pos);
        const base_command_ptr& at(const size_t pos) const;
        base_command_ptr& operator[](const size_t pos);
        const base_command_ptr& operator[](const size_t pos) const;

        void clear();

        iterator insert(const const_iterator& pos, const base_command_ptr& command);

        iterator erase(const const_iterator& pos);
        iterator erase(const const_iterator& pos, const const_iterator& pos2);

        void push_back(const base_command_ptr& command);
        void push_back(const std::vector<base_command_ptr>& command);

        void pop_back();
        void copy_from(const block_ptr& other);;

        block_state get_block_state() const;

        template <typename T>
        std::shared_ptr<T> get_command(const size_t i, const command_type command_assert = command_type::none) const;

        std::shared_ptr<class block_virt_ir> as_virt();
        std::shared_ptr<class block_x86_ir> as_x86();

    protected:
        ~block_ir();

        explicit block_ir(const block_state state, const uint32_t custom_block_id = UINT32_MAX);

        container_type commands_;
        base_command_ptr exit_cmd;
        block_state ir_state;

        virtual void handle_cmd_insert(const base_command_ptr& command = nullptr) = 0;
    };

    class block_virt_ir final : public block_ir
    {
    public:
        explicit block_virt_ir(const uint32_t custom_block_id = UINT32_MAX);

        cmd_vm_exit_ptr exit_as_vmexit() const;
        cmd_branch_ptr exit_as_branch() const;
        std::vector<block_ptr> get_calls() const;

    private:
        void handle_cmd_insert(const base_command_ptr& command) override;
    };

    class block_x86_ir final : public block_ir
    {
    public:
        explicit block_x86_ir(const uint32_t custom_block_id = UINT32_MAX);

        cmd_branch_ptr exit_as_branch() const;

    private:
        void handle_cmd_insert(const base_command_ptr& command) override;
    };

    using block_x86_ir_ptr = std::shared_ptr<class block_x86_ir>;
    using block_virt_ir_ptr = std::shared_ptr<class block_virt_ir>;
}
