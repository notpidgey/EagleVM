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

    void obfuscator::create_merged_handlers(const std::vector<block_ptr>& blocks)
    {
        const std::shared_ptr<trie_node_t> root_node = std::make_shared<trie_node_t>(0);
        for (const block_ptr& block : blocks)
        {
            for (int i = 0; i < block->size(); i++)
                root_node->add_children(block, i);
        }

        std::vector<std::pair<block_ptr, asmb::code_label_ptr>> generated_blocks;
        while (const auto result = root_node->find_path_max_depth(4))
        {
            auto [path_length, leaf] = result.value();
            const std::vector<std::shared_ptr<command_node_info_t>> command_branch = leaf->get_branch_similar_commands();

            // setup block to contain the cloned command
            block_virt_ir_ptr handler = std::make_shared<block_virt_ir>();
            handler->push_back(std::make_shared<cmd_ret>());

            std::weak_ptr start = leaf;
            std::shared_ptr<trie_node_t> curr;

            while (((curr = start.lock())) && curr != root_node)
            {
                handler->insert(handler->begin(), curr->command->clone());
                start = curr->parent;
            }

            // for each similar command list, we want to remove the commands from the block
            // we will replace it with a handler call
            std::unordered_map<block_ptr, uint32_t> encountered_blocks;
            for (auto& item : command_branch)
            {
                const auto block = item->block;
                if (encountered_blocks.contains(block))
                    continue;

                const auto start_idx = item->instruction_index - leaf->depth + 1;
                for (auto i = 0; i < path_length; i++)
                {
                    const auto idx = start_idx + i;
                    block->erase(block->begin() + idx);
                }

                // insert function call
                block->insert(block->begin() + start_idx, std::make_shared<cmd_call>(handler));

                // TODO: make sure to do something about this block.
                // it may come up again in similar commands and we do not want to remove from it again
                // this is a really shitty way with dealing it because we may have some non overlapping commands
                // that we should be able to merge
                // but i dont want to do it and im too lazy
                encountered_blocks.insert(block);
            }

            // remove all related commands because we are combining it into a handler
            for (auto& item : command_branch)
                root_node->erase_forwards(item->block, item->instruction_index - leaf->depth + 1);
        }

        __debugbreak();
    }
}
