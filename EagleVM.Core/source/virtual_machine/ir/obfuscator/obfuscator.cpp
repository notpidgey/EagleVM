#include "eaglevm-core/virtual_machine/ir/obfuscator/obfuscator.h"
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
    };

    class trie_node_t : public std::enable_shared_from_this<trie_node_t>
    {
    public:
        explicit trie_node_t(const size_t depth)
            : depth(depth)
        {
        }

        void add_children(const block_ptr& block, const uint16_t idx, const std::shared_ptr<command_node_info_t> previous = nullptr)
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

        void remove_branch(std::shared_ptr<command_node_info_t> node)
        {
            auto current = node;
            while (current && !current->previous.expired()) {
                const auto prev = current->previous.lock();
                prev->next.reset();
                current = prev;
            }

            current = node;
            while (current && !current->next.expired()) {
                const auto next = current->next.lock();
                next->previous.reset();
                current = next;
            }

            if (const auto parent = this->parent.lock()) {
                for (auto& [block, block_similars] : parent->similar_commands) {
                    auto it = std::ranges::find_if(block_similars,
                        [&node](const auto& cmd_info) { return cmd_info == node; });
                    if (it != block_similars.end()) {
                        block_similars.erase(it);
                        break;
                    }
                }
            }

            // Clean up empty entries in similar_commands
            for (auto it = similar_commands.begin(); it != similar_commands.end();) {
                if (it->second.empty()) {
                    it = similar_commands.erase(it);
                } else {
                    ++it;
                }
            }
        }

        std::vector<command_node_removal_t> get_branch()
        {
            std::vector<command_node_removal_t> branch;
            branch.reserve(similar_commands.size());

            for (auto& [block, block_similars] : similar_commands)
                for (const auto& similar : block_similars)
                    branch.emplace_back(block, similar->instruction_index - depth, depth + 1);

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
            // we reached a leaf
            if (children.empty())
            {
                // check constraints
                if (similar_commands.size() >= min_child_size)
                    return std::make_pair(1, shared_from_this());

                return std::nullopt;
            }

            std::optional<std::pair<size_t, std::shared_ptr<trie_node_t>>> max_path = std::nullopt;
            for (const auto& child : children)
            {
                if (const auto result_pair = child->find_path_max_depth(min_child_size))
                {
                    auto& [child_depth, found_child] = *result_pair;
                    if (!max_path || child_depth > max_path->first)
                        max_path = result_pair;
                }
            }

            return max_path;
        }

    private:
        size_t depth;

        base_command_ptr command = nullptr;
        std::unordered_map<block_ptr, std::vector<std::shared_ptr<command_node_info_t>>> similar_commands;

        std::weak_ptr<trie_node_t> parent;
        std::vector<std::shared_ptr<trie_node_t>> children;

        trie_node_t* find_similar_child(const base_command_ptr& command) const
        {
            for (const auto& child : children)
                if (child->command->get_command_type() == command->get_command_type() &&
                    child->command->is_similar(command))
                    return child.get();

            return nullptr;
        }

        static void update_existing_child(trie_node_t* child, block_ptr block, uint16_t idx,
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

            std::shared_ptr<command_node_info_t> child_command_info = std::make_shared<command_node_info_t>
                (block, idx, std::weak_ptr(previous), std::weak_ptr<command_node_info_t>());

            child->similar_commands[block].push_back(std::move(child_command_info));
            child->add_children(block, idx + 1, child_command_info);

            if (previous)
                previous->next = child_command_info;
        }

        void add_new_child(block_ptr block, uint16_t idx, const std::shared_ptr<command_node_info_t>& previous)
        {
            const std::shared_ptr<trie_node_t> new_child = std::make_shared<trie_node_t>(depth + 1);
            const std::shared_ptr<command_node_info_t> child_command_info = std::make_shared<command_node_info_t>
                (block, idx, std::weak_ptr(previous), std::weak_ptr<command_node_info_t>());

            new_child->parent = weak_from_this();
            new_child->command = block->at(idx);
            new_child->similar_commands[block].push_back(child_command_info);
            new_child->add_children(block, idx + 1, child_command_info);
            children.emplace_back(std::move(new_child));

            if (previous)
                previous->next = child_command_info;
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

        // test
        if (const auto result = root_node->find_path_max_similar(4))
        {
            auto [similar, leaf] = result.value();
            auto branch = leaf->get_branch();

            for (auto& item : branch)
                root_node->



        }
    }
}
