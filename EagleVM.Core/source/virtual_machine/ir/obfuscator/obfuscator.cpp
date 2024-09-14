#include "eaglevm-core/virtual_machine/ir/obfuscator/obfuscator.h"

#include <deque>
#include <ranges>

#include "eaglevm-core/virtual_machine/ir/ir_translator.h"

namespace eagle::ir
{
    void obfuscator::run_preopt_pass(const preopt_block_vec& preopt_vec, const std::unique_ptr<dasm::analysis::liveness>& liveness)
    {
        for (const auto& preopt_block : preopt_vec)
        {
            VM_ASSERT(liveness->live.contains(preopt_block->original_block), "liveness data must contain data for existing block");
            const auto [block_in, block_out] = liveness->live[preopt_block->original_block];
            auto& ir_body = preopt_block->body;

            std::shared_ptr<block_ir> false_in = std::make_shared<block_ir>(vm_block);
            std::shared_ptr<block_ir> false_out = std::make_shared<block_ir>(vm_block);

            for (int k = ZYDIS_REGISTER_RAX; k <= ZYDIS_REGISTER_R15; k++)
            {
                if (0 == block_in.get_gpr64(static_cast<codec::reg>(k)))
                {
                    false_in->push_back(std::make_shared<cmd_push>(util::get_ran_device().gen_16()));
                    false_in->push_back(std::make_shared<cmd_context_store>(static_cast<codec::reg>(k), codec::reg_size::bit_64));
                }

                if (0 == block_out.get_gpr64(static_cast<codec::reg>(k)))
                {
                    false_out->push_back(std::make_shared<cmd_push>(util::get_ran_device().gen_16()));
                    false_out->push_back(std::make_shared<cmd_context_store>(static_cast<codec::reg>(k), codec::reg_size::bit_64));
                }
            }

            // false in
            const auto next_cmd_type = ir_body.front()->at(0)->get_command_type();
            if (next_cmd_type == command_type::vm_exec_x86 || next_cmd_type == command_type::vm_exec_dynamic_x86)
                false_in->push_back(std::make_shared<cmd_vm_exit>());

            false_in->push_back(std::make_shared<cmd_branch>(ir_body.front(), exit_condition::jmp));
            ir_body.insert(ir_body.begin(), false_in);

            // false out
            // this code is fucking insane
            VM_ASSERT(ir_body.back()->size() >= 2, "back block must  contain greater than 2 elements");

            const auto back_command_type = ir_body.back()->at(ir_body.size() - 2)->get_command_type();
            if (back_command_type != command_type::vm_exec_dynamic_x86 && back_command_type != command_type::vm_exec_x86)
            {
            }
        }
    }

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

        explicit trie_node_t(const size_t depth)
            : depth(depth)
        {
        }

        void add_children(const block_ptr& block, const uint16_t idx, const std::shared_ptr<command_node_info_t>& previous = nullptr)
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

        void erase_forwards(const block_ptr& ptr, const size_t start_idx)
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

        std::vector<std::shared_ptr<command_node_info_t>> get_branch()
        {
            std::vector<std::shared_ptr<command_node_info_t>> branch;
            branch.reserve(similar_commands.size());

            for (auto& block_similars : similar_commands | std::views::values)
                for (const auto& similar : block_similars)
                    branch.push_back(similar);

            return branch;
        }

        std::optional<std::pair<size_t, std::shared_ptr<trie_node_t>>> find_path_max_similar(const size_t min_depth)
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

        std::optional<std::pair<size_t, std::shared_ptr<trie_node_t>>> find_path_max_depth(const size_t min_child_size)
        {
            std::optional<std::pair<size_t, std::shared_ptr<trie_node_t>>> max_path = std::nullopt;

            // Check all children
            for (const auto& child : children)
            {
                if (const auto result_pair = child->find_path_max_depth(min_child_size))
                {
                    auto& [child_depth, found_child] = *result_pair;
                    if (!max_path || child_depth > max_path->first)
                        max_path = result_pair;
                }
            }

            // Check the current node
            if (count_similar_commands() >= min_child_size)
            {
                if (!max_path || depth > max_path->first)
                    max_path = std::make_pair(depth, shared_from_this());
            }

            return max_path;
        }

    private:
        base_command_ptr command = nullptr;
        std::unordered_map<block_ptr, std::vector<std::shared_ptr<command_node_info_t>>> similar_commands;

        std::weak_ptr<trie_node_t> parent;
        std::vector<std::shared_ptr<trie_node_t>> children;

        std::shared_ptr<trie_node_t> find_similar_child(const base_command_ptr& command) const
        {
            for (const auto& child : children)
                if (child->command->get_command_type() == command->get_command_type() &&
                    child->command->is_similar(command))
                    return child;

            return nullptr;
        }

        void update_existing_child(const std::shared_ptr<trie_node_t>& child, const block_ptr& block, const uint16_t idx,
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

        void add_new_child(const block_ptr& block, const uint16_t idx, const std::shared_ptr<command_node_info_t>& previous)
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

        size_t count_similar_commands() const
        {
            size_t total = 0;
            for (const auto& cmd_infos : similar_commands | std::views::values)
                total += cmd_infos.size();

            return total;
        }
    };

    void obfuscator::create_merged_handlers(const std::vector<block_ptr>& blocks)
    {
        const std::shared_ptr<trie_node_t> root_node = std::make_shared<trie_node_t>(0);
        for (const block_ptr& block : blocks)
        {
            for (int i = 0; i < block->size(); i++)
                root_node->add_children(block, i);
        }

        size_t result_count = 0;
        while (const auto result = root_node->find_path_max_depth(4))
        {
            auto [similar, leaf] = result.value();
            const auto branch = leaf->get_branch();

            // remove all related commands because we are combining it into a handler
            for (auto& item : branch)
                root_node->erase_forwards(item->block, item->instruction_index - leaf->depth + 1);

            result_count++;
        }

        __debugbreak();
    }
}
