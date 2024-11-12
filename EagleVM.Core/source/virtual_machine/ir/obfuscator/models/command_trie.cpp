#include "eaglevm-core/virtual_machine/ir/obfuscator/models/command_trie.h"

#include <unordered_set>
#include <deque>
#include <ranges>

namespace eagle::ir
{
    trie_node_t::trie_node_t(const size_t depth): depth(depth), command(nullptr)
    {
    }

    void trie_node_t::add_children(const block_ptr& block, const uint16_t idx, const std::shared_ptr<command_node_info_t>& previous)
    {
        if (block->size() == idx)
            return;

        const auto cmd = block->at(idx);
        if (cmd->get_command_type() == command_type::vm_enter ||
            cmd->get_command_type() == command_type::vm_exit)
            return;

        if (const auto existing_child = find_similar_child(block->at(idx)))
            update_existing_child(existing_child, block, idx, previous);
        else
            add_new_child(block, idx, previous);
    }

    void trie_node_t::erase_forwards(const block_ptr& ptr, const size_t start_idx)
    {
        // erase forwards
        std::deque<std::shared_ptr<trie_node_t>> search_nodes;
        if (depth == 0) // root node
            for (auto& child : children)
                search_nodes.emplace_back(child);
        else
            search_nodes.emplace_back(shared_from_this());

        while (!search_nodes.empty())
        {
            const auto layer_nodes = search_nodes.size();
            for (auto i = 0; i < layer_nodes; i++)
            {
                std::unordered_set<std::shared_ptr<trie_node_t>> next_layer;

                const std::shared_ptr<trie_node_t> search_node = search_nodes.front();
                if (!search_node->similar_commands.contains(ptr))
                {
                    search_nodes.pop_front();
                    continue;
                }

                auto& block_commands = search_node->similar_commands[ptr];
                for (auto j = 0; j < block_commands.size();)
                {
                    const auto cmd = block_commands[j];
                    if (cmd->instruction_index >= start_idx)
                        block_commands.erase(block_commands.begin() + j);
                    else
                        j++;

                    if (const auto next = cmd->next.lock())
                        next_layer.insert(next->current_node.lock());
                }


                search_nodes.insert(search_nodes.end(), next_layer.begin(), next_layer.end());
                search_nodes.pop_front();
            }
        }
    }

    std::vector<std::shared_ptr<command_node_info_t>> trie_node_t::get_branch_similar_commands()
    {
        std::vector<std::shared_ptr<command_node_info_t>> branch;
        branch.reserve(similar_commands.size());

        for (auto& block_similars : similar_commands | std::views::values)
            for (const auto& similar : block_similars)
                branch.push_back(similar);

        return branch;
    }

    std::optional<std::pair<size_t, std::shared_ptr<trie_node_t>>> trie_node_t::find_path_max_similar(const size_t min_depth)
    {
        if (children.empty())
        {
            if (depth >= min_depth)
                return std::make_pair(similar_commands.size(), shared_from_this());

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
            auto current_similar = similar_commands.size();
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

    std::shared_ptr<trie_node_t> trie_node_t::find_similar_child(const base_command_ptr& command) const
    {
        for (const auto& child : children)
            if (child->command->get_command_type() == command->get_command_type() &&
                child->command->is_similar(command))
                return child;

        return nullptr;
    }

    void trie_node_t::update_existing_child(const std::shared_ptr<trie_node_t>& child, const block_ptr& block, const uint16_t idx,
        const std::shared_ptr<command_node_info_t>& previous)
    {
        // Check if this block already has an entry in similar_commands
        const auto it = child->similar_commands.find(block);
        if (it != child->similar_commands.end())
        {
            const auto& entries = it->second;
            for (const auto& entry : entries)
            {
                if (entry->instruction_index <= idx)
                {
                    // We've found an existing entry that starts at or before this index
                    // Don't add a new entry to prevent overlap
                    return;
                }
            }
        }

        const std::shared_ptr<command_node_info_t> cmd_info = std::make_shared<command_node_info_t>();
        cmd_info->block = block;
        cmd_info->instruction_index = idx;
        cmd_info->previous = std::weak_ptr(previous);
        cmd_info->next = std::weak_ptr<command_node_info_t>();
        cmd_info->current_node = child;

        child->similar_commands[block].push_back(cmd_info);
        child->add_children(block, idx + 1, cmd_info);

        if (previous)
            previous->next = cmd_info;
    }

    void trie_node_t::add_new_child(const block_ptr& block, const uint16_t idx, const std::shared_ptr<command_node_info_t>& previous)
    {
        const std::shared_ptr<trie_node_t> new_child = std::make_shared<trie_node_t>(depth + 1);
        const std::shared_ptr<command_node_info_t> cmd_info = std::make_shared<command_node_info_t>();
        cmd_info->block = block;
        cmd_info->instruction_index = idx;
        cmd_info->previous = std::weak_ptr(previous);
        cmd_info->next = std::weak_ptr<command_node_info_t>();
        cmd_info->current_node = new_child;

        new_child->parent = weak_from_this();
        new_child->command = block->at(idx);
        new_child->similar_commands[block].push_back(cmd_info);
        new_child->add_children(block, idx + 1, cmd_info);
        children.emplace_back(new_child);

        if (previous)
            previous->next = cmd_info;
    }

    size_t trie_node_t::count_similar_commands() const
    {
        size_t total = 0;
        for (const auto& cmd_infos : similar_commands | std::views::values)
            total += cmd_infos.size();

        return total;
    }
}
