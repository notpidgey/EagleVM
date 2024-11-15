#include "eaglevm-core/virtual_machine/ir/obfuscator/obfuscator.h"

#include <deque>
#include <ranges>

#include "eaglevm-core/virtual_machine/ir/ir_translator.h"
#include "eaglevm-core/virtual_machine/ir/obfuscator/models/command_trie.h"

namespace eagle::ir
{
    void obfuscator::run_preopt_pass(const preopt_block_vec& preopt_vec, const dasm::analysis::liveness* liveness)
    {
        for (const auto& preopt_block : preopt_vec)
        {
            VM_ASSERT(liveness->live.contains(preopt_block->original_block), "liveness data must contain data for existing block");
            const auto [block_in, block_out] = liveness->live.at(preopt_block->original_block);

            block_virt_ir_ptr first_block = nullptr;
            for (auto& body : preopt_block->body)
                if (body->get_block_state() == vm_block)
                    first_block = std::static_pointer_cast<block_virt_ir>(body);

            if (first_block == nullptr)
                continue;

            auto in_insert_index = 0;
            if (first_block->at(0)->get_command_type() == command_type::vm_enter)
                in_insert_index = 1;

            for (int k = ZYDIS_REGISTER_RAX; k <= ZYDIS_REGISTER_R15; k++)
            {
                if (0 == block_in.get_gpr64(static_cast<codec::reg>(k)))
                {
                    first_block->insert(first_block->begin() + in_insert_index,
                        std::make_shared<cmd_context_store>(static_cast<codec::reg>(k), codec::reg_size::bit_64));
                    first_block->insert(first_block->begin() + in_insert_index,
                        std::make_shared<cmd_push>(util::get_ran_device().gen_16(), ir_size::bit_64));
                }

                if (0 == block_out.get_gpr64(static_cast<codec::reg>(k)))
                {
                    first_block->insert(first_block->end() - 1,
                        std::make_shared<cmd_context_store>(static_cast<codec::reg>(k), codec::reg_size::bit_64));
                    first_block->insert(first_block->end() - 1, std::make_shared<cmd_push>(util::get_ran_device().gen_16(), ir_size::bit_64));
                }
            }
        }
    }

    std::vector<block_ptr> obfuscator::create_merged_handlers(const std::vector<block_ptr>& blocks)
    {
        std::shared_ptr<trie_node_t> root_node = std::make_shared<trie_node_t>(0);
        for (const block_ptr& block : blocks)
        {
            for (int i = 0; i < block->size(); i++)
                root_node->add_children(block, i);
        }

        {
            std::ostringstream out;
            root_node->print(out);

            std::cout << out.str() << std::flush;
        }

        std::vector<block_ptr> generated_handlers;
        while (const auto result = root_node->find_path_max_depth(4))
        {
            auto [path_length, leaf] = result.value();
            const std::vector<std::shared_ptr<command_node_info_t>> block_occurrences = leaf->get_branch_similar_commands();

            // setup block to contain the cloned command
            block_virt_ir_ptr merge_handler = std::make_shared<block_virt_ir>();
            merge_handler->push_back(std::make_shared<cmd_ret>());

            std::weak_ptr curr_it = leaf;
            std::shared_ptr<trie_node_t> curr;

            while (((curr = curr_it.lock())) && curr != root_node)
            {
                merge_handler->insert(merge_handler->begin(), curr->command->clone());
                curr_it = curr->parent;
            }

            std::unordered_map<block_ptr, std::vector<size_t>> block_info;
            for (auto& cmd : block_occurrences)
            {
                size_t start = cmd->instruction_index;
                size_t end = start + path_length;

                bool any_overlap = false;

                for (auto& index : block_info[cmd->block])
                {
                    size_t exist_start = index;
                    size_t exist_end = index + path_length;

                    // [start, end)
                    // [exist_start, exist_end)
                    // check if they overlap

                    if (std::max(start, exist_start) < std::min(end, exist_end))
                    {
                        any_overlap = true;
                        break;
                    }
                }

                if (any_overlap)
                    continue;

                const auto deletion_idx = cmd->instruction_index - (path_length - 1);
                for (int i = path_length - 1; i >= 0; i--)
                    cmd->block->erase(cmd->block->begin() + deletion_idx);

                cmd->block->insert(cmd->block->begin() + deletion_idx, std::make_shared<cmd_call>(merge_handler));
                block_info[cmd->block].push_back(start);
            }

            // this is actually not quite as easy as erasing, the best thing we can do right now is just
            // rebuild the tree
            // // remove all related commands because we are combining it into a handler
            // for (auto& item : block_occurrences)
            //     root_node->erase_forwards(item->block, item->instruction_index - (path_length - 1), path_length);

            root_node = std::make_shared<trie_node_t>(0);
            for (const block_ptr& block : blocks)
                for (int i = 0; i < block->size(); i++)
                    root_node->add_children(block, i);

            generated_handlers.emplace_back(merge_handler);
        }

        return generated_handlers;
    }
}
