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
        size_t instruction_index = 0;
    };

    using command_node_removal_ptr = std::shared_ptr<command_node_removal_t>;
    using command_node_info_ptr = std::shared_ptr<command_node_info_t>;
    using trie_node_ptr = std::shared_ptr<class trie_node_t>;

    class trie_node_t : public std::enable_shared_from_this<trie_node_t>
    {
    public:
        uint32_t uuid;
        size_t depth;

        base_command_ptr command;
        std::vector<command_node_info_ptr> matched_commands;

        std::weak_ptr<trie_node_t> parent;
        std::vector<trie_node_ptr> children;

        explicit trie_node_t(size_t depth);

        void add_children(const block_ptr& block, uint16_t idx);
        void erase_forwards(const block_ptr& ptr, size_t start_idx, size_t length);

        std::vector<std::shared_ptr<command_node_info_t>> get_branch_similar_commands();

        /**
         * searchers for a path where the instance count of the pattern occurrence is maximized.
         * this means we ignore the depth, and only care about optimizing out the greatest pattern
         * @param min_depth the minimum depth the pattern must start at
         */
        std::optional<std::pair<size_t, trie_node_ptr>> find_path_max_similar(size_t min_depth);

        /**
         * searches for a path where the depth is maximized. this means the result may contain very few matching instances
         * of this pattern, but the pattern will be long.
         * @param min_child_size the minimum instances of this pattern match that may occur
         */
        std::optional<std::pair<size_t, trie_node_ptr>> find_path_max_depth(size_t min_child_size);

        bool print(std::ostringstream& out);

    private:
        trie_node_ptr find_similar_child(const base_command_ptr& cmd) const;

        void update_existing_child(const trie_node_ptr& child, const block_ptr& block, uint16_t idx);
        void add_new_child(const block_ptr& block, uint16_t idx);

        size_t count_similar_commands() const;
    };
}
