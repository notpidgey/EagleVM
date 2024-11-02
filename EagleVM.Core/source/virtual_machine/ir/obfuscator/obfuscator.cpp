#include "eaglevm-core/virtual_machine/ir/obfuscator/obfuscator.h"

#include <deque>
#include <ranges>

#include "eaglevm-core/virtual_machine/ir/ir_translator.h"
#include "eaglevm-core/virtual_machine/ir/obfuscator/models/command_trie.h"

namespace eagle::ir
{
    void obfuscator::run_preopt_pass(const preopt_block_vec& preopt_vec, const std::unique_ptr<dasm::analysis::liveness>& liveness)
    {
        for (const auto& preopt_block : preopt_vec)
        {
            VM_ASSERT(liveness->live.contains(preopt_block->original_block), "liveness data must contain data for existing block");
            const auto [block_in, block_out] = liveness->live[preopt_block->original_block];
            auto& ir_body = preopt_block->body;

            block_virt_ir_ptr false_in = std::make_shared<block_virt_ir>(vm_block);
            block_virt_ir_ptr false_out = std::make_shared<block_virt_ir>(vm_block);

            for (int k = ZYDIS_REGISTER_RAX; k <= ZYDIS_REGISTER_R15; k++)
            {
                if (0 == block_in.get_gpr64(static_cast<codec::reg>(k)))
                {
                    //false_in->push_back(std::make_shared<cmd_push>(util::get_ran_device().gen_16(), ir_size::bit_64));
                    //false_in->push_back(std::make_shared<cmd_context_store>(static_cast<codec::reg>(k), codec::reg_size::bit_64));
                }

                if (0 == block_out.get_gpr64(static_cast<codec::reg>(k)))
                {
                    //false_out->push_back(std::make_shared<cmd_push>(util::get_ran_device().gen_16(), ir_size::bit_64));
                    //false_out->push_back(std::make_shared<cmd_context_store>(static_cast<codec::reg>(k), codec::reg_size::bit_64));
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

        std::vector<std::vector<std::shared_ptr<command_node_info_t>>> command_groups;
        while (const auto result = root_node->find_path_max_depth(4))
        {
            auto [similar, leaf] = result.value();
            const auto similar_commands = leaf->get_branch_similar_commands();

            // remove all related commands because we are combining it into a handler
            for (auto& item : similar_commands)
                root_node->erase_forwards(item->block, item->instruction_index - leaf->depth + 1);

            command_groups.push_back(similar_commands);
        }

       __debugbreak();
    }
}
