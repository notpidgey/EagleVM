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

    struct command_node_info_t
    {
        bool last = false;
        uint16_t instruction_index = 0;
        block_ptr block = nullptr;
    };

    enum class search_option
    {
        option_none,
        option_depth,
        option_children
    };

    class trie_node_t : public std::enable_shared_from_this<trie_node_t>
    {
    public:
        size_t depth;

        base_command_ptr command = nullptr;
        std::unordered_map<block_ptr, std::vector<command_node_info_t>> similar_commands;
        std::vector<std::shared_ptr<trie_node_t>> children;

        std::weak_ptr<trie_node_t> parent;

        void add_children(const block_ptr& block, const uint16_t idx)
        {
            if (block->size() == idx)
                return;

            const auto cmd = block->at(idx);
            if (cmd->get_command_type() == command_type::vm_enter ||
                cmd->get_command_type() == command_type::vm_exit)
                return;

            if (const auto existing_child = find_similar_child(block->at(idx)))
                update_existing_child(existing_child, block, idx);
            else
                add_new_child(block, idx);
        }

        std::optional<std::pair<size_t, std::shared_ptr<trie_node_t>>> find_path_max_similar(const size_t min_depth)
        {
            if (children.empty())
            {
                if (depth >= min_depth)
                    return std::make_pair(this->similar_commands.size(), shared_from_this());

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
                if (depth >= min_depth && max_path->first < similar_commands.size())
                    return std::make_pair(this->similar_commands.size(), shared_from_this());
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
        trie_node_t* find_similar_child(const base_command_ptr& command) const
        {
            for (const auto& child : children)
                if (child->command->get_command_type() == command->get_command_type() &&
                    child->command->is_similar(command))
                    return child.get();

            return nullptr;
        }

        static void update_existing_child(trie_node_t* child, block_ptr block, uint16_t idx)
        {
            child->similar_commands[block].emplace_back(block->size() - 1 == idx, idx, block);
            child->add_children(block, idx + 1);
        }

        void add_new_child(block_ptr block, uint16_t idx)
        {
            const auto new_child = std::make_shared<trie_node_t>();
            new_child->parent = weak_from_this();
            new_child->command = block->at(idx);
            new_child->depth = depth + 1;
            new_child->similar_commands[block].emplace_back(block->size() - 1 == idx, idx, block);
            new_child->add_children(block, idx + 1);

            children.emplace_back(std::move(new_child));
        }

        size_t count_unoverlapped(block_ptr block)
        {
            for (auto& [block, commands] : similar_commands)
            {
            }
        }
    };

    void obfuscator::create_merged_handlers(const std::vector<block_ptr>& blocks)
    {
        const std::shared_ptr<trie_node_t> root_node = std::make_shared<trie_node_t>();
        root_node->depth = 1;

        for (const block_ptr& block : blocks)
        {
            for (int i = 0; i < block->size(); i++)
                root_node->add_children(block, i);
        }

        // test
        if (const auto result = root_node->find_path_max_similar(5))
        {
            auto val = result.value();
        }
    }
}
