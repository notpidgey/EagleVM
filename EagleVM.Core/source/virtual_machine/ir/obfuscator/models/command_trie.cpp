#include "eaglevm-core/virtual_machine/ir/obfuscator/models/command_trie.h"

#include <unordered_set>
#include <deque>
#include <ranges>

namespace eagle::ir
{
    trie_node_t::trie_node_t(const size_t depth): depth(depth), command(nullptr)
    {
    }

    void trie_node_t::add_children(const block_ptr& block, const uint16_t idx)
    {
        if (block->size() == idx)
            return;

        const auto cmd = block->at(idx);
        if (cmd->get_command_type() == command_type::vm_exit || cmd->get_command_type() == command_type::vm_ret
            || cmd->get_command_type() == command_type::vm_call || cmd->get_command_type() == command_type::vm_branch || cmd->get_command_type() ==
            command_type::vm_enter)
            return;

        if (const auto existing_child = find_similar_child(block->at(idx)))
            update_existing_child(existing_child, block, idx);
        else
            add_new_child(block, idx);
    }

    void trie_node_t::erase_forwards(const block_ptr& ptr, const size_t start_idx, const size_t length)
    {
        const size_t end_idx = start_idx + length;
        auto it = matched_commands.begin();

        while (it != matched_commands.end())
        {
            const auto& cmd_info = *it;
            if (cmd_info->block == ptr)
            {
                if (cmd_info->instruction_index >= start_idx && cmd_info->instruction_index < end_idx)
                {
                    it = matched_commands.erase(it);
                    continue;
                }
            }

            ++it;
        }

        for (const auto& child : children)
            child->erase_forwards(ptr, start_idx, length);
    }

    std::vector<command_node_info_ptr> trie_node_t::get_branch_similar_commands()
    {
        return matched_commands;
    }

    std::optional<std::pair<size_t, std::shared_ptr<trie_node_t>>> trie_node_t::find_path_max_similar(const size_t min_depth)
    {
        if (children.empty())
        {
            if (depth >= min_depth)
                return std::make_pair(matched_commands.size(), shared_from_this());

            return std::nullopt;
        }

        std::optional<std::pair<size_t, std::shared_ptr<trie_node_t>>> max_path = std::nullopt;
        for (const auto& child : children)
        {
            if (const auto result_pair = child->find_path_max_similar(min_depth))
            {
                auto& [similar_count, found_child] = *result_pair;
                if (!max_path || similar_count > max_path->first)
                    max_path = result_pair;
            }
        }

        if (max_path)
        {
            // we want to check if the current node meets the conditions better
            auto current_similar = matched_commands.size();
            if (depth >= min_depth && max_path->first < current_similar)
                return std::make_pair(current_similar, shared_from_this());
        }

        return max_path;
    }

    std::optional<std::pair<size_t, std::shared_ptr<trie_node_t>>> trie_node_t::find_path_max_depth(const size_t min_child_size)
    {
        std::optional<std::pair<size_t, std::shared_ptr<trie_node_t>>> max_path = std::nullopt;

        // check all children
        for (const auto& child : children)
        {
            if (const auto result_pair = child->find_path_max_depth(min_child_size))
            {
                auto& [child_depth, found_child] = *result_pair;
                if (!max_path || child_depth > max_path->first)
                    max_path = result_pair;
            }
        }

        // check the current node
        if (count_similar_commands() >= min_child_size)
        {
            if (!max_path || depth > max_path->first)
                max_path = std::make_pair(depth, shared_from_this());
        }

        return max_path;
    }

    bool trie_node_t::print(std::ostringstream& out)
    {
        std::unordered_set<std::string> added_nodes;

        if (depth == 0)
            out << "digraph Trie {\n  node [shape=box, fontname=\"Courier\"];\n";

        if (this->depth != 0 && matched_commands.size() < 2)
            return false;

        const std::string node_id = "node_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        out << "    " << node_id << " [label=<";
        if (command)
            out << "<b>Command: " << command->to_string() << "</b><br/>";
        out << "Depth: " << depth
            << "<br/>Matched Commands: " << matched_commands.size()
            << ">];\n";

        for (const auto& child : children)
        {
            if (!child) continue;

            const bool result = child->print(out);
            if (!result)
                continue;

            out << "    " << node_id << " -> " << "node_" + std::to_string(reinterpret_cast<uintptr_t>(child.get())) << ";\n";
        }

        if (depth == 0)
            out << "}\n";

        return true;
    }

    std::shared_ptr<trie_node_t> trie_node_t::find_similar_child(const base_command_ptr& command) const
    {
        for (const auto& child : children)
            if (child->command->is_similar(command))
                return child;

        return nullptr;
    }

    void trie_node_t::update_existing_child(const std::shared_ptr<trie_node_t>& child, const block_ptr& block, const uint16_t idx)
    {
        // Check if this block already has an entry in similar_commands
        const auto it = std::ranges::find_if(child->matched_commands, [&](auto& item)
        {
            return item->block == block && item->instruction_index == idx;
        });

        if (it != child->matched_commands.end())
            return;

        const command_node_info_ptr cmd_info = std::make_shared<command_node_info_t>();
        cmd_info->block = block;
        cmd_info->instruction_index = idx;

        child->matched_commands.push_back(cmd_info);
        child->add_children(block, idx + 1);
    }

    void trie_node_t::add_new_child(const block_ptr& block, const uint16_t idx)
    {
        const auto new_child = std::make_shared<trie_node_t>(depth + 1);
        const auto cmd_info = std::make_shared<command_node_info_t>();
        cmd_info->block = block;
        cmd_info->instruction_index = idx;

        new_child->parent = weak_from_this();
        new_child->command = block->at(idx)->clone();
        new_child->matched_commands.push_back(cmd_info);

        children.push_back(new_child);
        new_child->add_children(block, idx + 1);
    }

    size_t trie_node_t::count_similar_commands() const
    {
        return matched_commands.size();
    }
}
