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
        base_command_ptr command;
        std::weak_ptr<trie_node_t> parent;

        explicit trie_node_t(size_t depth);

        void add_children(const block_ptr& block, uint16_t idx, const std::shared_ptr<command_node_info_t>& previous = nullptr);
        void erase_forwards(const block_ptr& ptr, size_t start_idx);

        std::vector<std::shared_ptr<command_node_info_t>> get_branch_similar_commands();


        /**
         * searchers for a path where the instance count of the pattern occurrence is maximized.
         * this means we ignore the depth, and only care about optimizing out the greatest pattern
         * @param min_depth the minimum depth the pattern must start at
         */
        std::optional<std::pair<size_t, std::shared_ptr<trie_node_t>>> find_path_max_similar(size_t min_depth);

        /**
         * searches for a path where the depth is maximized. this means the result may contain very few matching instances
         * of this pattern, but the pattern will be long.
         * @param min_child_size the minimum instances of this pattern match that may occur
         */
        std::optional<std::pair<size_t, std::shared_ptr<trie_node_t>>> find_path_max_depth(size_t min_child_size);

    private:
        std::unordered_map<block_ptr, std::vector<std::shared_ptr<command_node_info_t>>> similar_commands;

        std::vector<std::shared_ptr<trie_node_t>> children;

        std::shared_ptr<trie_node_t> find_similar_child(const base_command_ptr& command) const;
        void update_existing_child(const std::shared_ptr<trie_node_t>& child, const block_ptr& block, uint16_t idx,
            const std::shared_ptr<command_node_info_t>& previous);
        void add_new_child(const block_ptr& block, uint16_t idx, const std::shared_ptr<command_node_info_t>& previous);
        size_t count_similar_commands() const;
    };
}
