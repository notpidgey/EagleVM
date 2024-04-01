#include "eaglevm-core/virtual_machine/il/il_translator.h"
#include "eaglevm-core/virtual_machine/il/il_bb.h"

#include "eaglevm-core/virtual_machine/il/commands/include.h"

#include "eaglevm-core/disassembler/disassembler.h"

namespace eagle::il
{
    il_translator::il_translator(dasm::segment_dasm* seg_dasm)
    {
        dasm = seg_dasm;
    }

    std::vector<il_bb_ptr> il_translator::translate(const std::vector<dasm::basic_block*>& blocks)
    {
        // we want to initialzie the entire map with bb translates
        for (dasm::basic_block* block : blocks)
            bb_map[block] = std::make_shared<il_bb>();

        std::vector<il_bb_ptr> translated_blocks;
        for (dasm::basic_block* block : blocks)
        {
            const std::vector<il_bb_ptr> translated_block = translate(block);
            translated_blocks.append_range(translated_block);
        }

        return translated_blocks;
    }

    std::vector<il_bb_ptr> il_translator::translate(dasm::basic_block* bb)
    {
        if (bb->decoded_insts.empty())
            return {};

        // create block now
        const il_bb_ptr exit_block = std::make_shared<il_bb>();

        // bb must exist in bb_map
        const il_bb_ptr block_body = bb_map[bb];
        block_body->set_exit(std::make_shared<cmd_exit>(exit_block, exit_condition::jump));

        // we calculate skips here because a basic block might end with a jump
        // we will handle that manually instead of letting the il translator handle this
        const dasm::block_end_reason end_reason = bb->get_end_reason();
        const uint8_t skips = end_reason == dasm::block_end ? 0 : 1;

        for (uint32_t i = 0; i < bb->decoded_insts.size() - skips; i++)
        {
            // use il x86 translator to translate the instruction to il
        }

        // now that all virtualized instructions have been added, we know if we are in the vm
        // if we are in the vm, depending on the kind of jump, we can jump right into the code
        // however, we might not be able to depending on the type of VM the block uses
        // this will be implemented later though
        switch (bb->get_end_reason())
        {
            case dasm::block_end:
            case dasm::block_jump:
            {
                auto [target, type] = dasm->get_jump(bb);
                if (type == dasm::jump_outside_segment)
                {
                    // return back to .text
                    exit_block->set_exit(std::make_shared<cmd_exit>(target, exit_condition::jump));
                }
                else
                {
                    // this block exits to another block we are virtualizing
                    exit_block->set_exit(std::make_shared<cmd_exit>(bb_map[bb], exit_condition::jump));
                }
            }
            break;
            case dasm::block_conditional_jump:
            {
                // we will have two jump results (first, last)
                std::vector<exit_result> results = {};
                {
                    auto [target, type] = dasm->get_jump(bb);
                    if (type == dasm::jump_outside_segment)
                        results.emplace_back(target);
                    else
                        results.emplace_back(bb_map[bb]);
                }
                {
                    auto [target, type] = dasm->get_jump(bb, true);
                    if (type == dasm::jump_outside_segment)
                        results.emplace_back(target);
                    else
                        results.emplace_back(bb_map[bb]);
                }

                exit_block->set_exit(std::make_shared<cmd_exit>(results, exit_condition::conditional));
            }
            break;
        }

        // theres potential here with a exit_block which is only jumped to if only the next block is
        // another virtualized block

        const il_bb_ptr entry_block = std::make_shared<il_bb>();
        entry_block->add_command(std::make_shared<cmd_enter>());
        entry_block->set_exit(std::make_shared<cmd_exit>(block_body, exit_condition::jump));

        // the reason for having these 3 blocks is
        // entry_block -> block_body -> exit_block
        // if we join another block which is still inside the vm

        return {block_body, entry_block, exit_block};
    }

    std::vector<il_bb_ptr> il_translator::optimize(std::vector<il_bb_ptr> blocks)
    {
        // the idea of this function is that we want to walk every block
        // for every block, we want to check the block that is being jumped to
        // we want to see if vmenter is ever being used for that block
        // if its not, it can be optimized out and vmexits for blocks that jump to it can be removed as well
        return {};
    }
}
