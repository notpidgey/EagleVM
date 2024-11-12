#pragma once
#include <memory>
#include "eaglevm-core/virtual_machine/ir/block.h"

namespace eagle::ir
{
    struct command_node_removal_t
    {
        block_ptr block;

        size_t start_idx;
        size_t size;
    };

    struct command_node_info_t
    {
        block_ptr block = nullptr;
        uint16_t instruction_index = 0;

        std::weak_ptr<command_node_info_t> previous;
        std::weak_ptr<command_node_info_t> next;

        std::weak_ptr<class trie_node_t> current_node;
    };

    class trie_node_t : public std::enable_shared_from_this<trie_node_t>
    {
    public:
        size_t depth;

        explicit trie_node_t(size_t depth);

        void add_children(const block_ptr& block, uint16_t idx, const std::shared_ptr<command_node_info_t>& previous = nullptr);
        void erase_forwards(const block_ptr& ptr, size_t start_idx);

        std::vector<std::shared_ptr<command_node_info_t>> get_branch_similar_commands();
        std::optional<std::pair<size_t, std::shared_ptr<trie_node_t>>> find_path_max_similar(size_t min_depth);
        std::optional<std::pair<size_t, std::shared_ptr<trie_node_t>>> find_path_max_depth(size_t min_child_size);

    private:
        base_command_ptr command = nullptr;
        std::unordered_map<block_ptr, std::vector<std::shared_ptr<command_node_info_t>>> similar_commands;

        std::weak_ptr<trie_node_t> parent;
        std::vector<std::shared_ptr<trie_node_t>> children;

        std::shared_ptr<trie_node_t> find_similar_child(const base_command_ptr& command) const;
        void update_existing_child(const std::shared_ptr<trie_node_t>& child, const block_ptr& block, uint16_t idx,
            const std::shared_ptr<command_node_info_t>& previous);
        void add_new_child(const block_ptr& block, uint16_t idx, const std::shared_ptr<command_node_info_t>& previous);
        size_t count_similar_commands() const;
    };
}
