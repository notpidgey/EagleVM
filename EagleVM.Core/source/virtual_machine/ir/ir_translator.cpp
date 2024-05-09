#include "eaglevm-core/virtual_machine/ir/ir_translator.h"

#include "eaglevm-core/virtual_machine/ir/commands/include.h"
#include "eaglevm-core/virtual_machine/ir/block.h"

#include "eaglevm-core/disassembler/disassembler.h"
#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/virtual_machine/ir/x86/handler_data.h"

namespace eagle::il
{
    ir_translator::ir_translator(dasm::segment_dasm* seg_dasm)
    {
        dasm = seg_dasm;
    }

    std::vector<block_il_ptr> ir_translator::translate(const std::vector<dasm::basic_block*>& asm_blocks)
    {
        // we want to initialzie the entire map with bb translates
        for (dasm::basic_block* block : asm_blocks)
            bb_map[block] = std::make_shared<block_il>();

        std::vector<block_il_ptr> translated_blocks;
        for (dasm::basic_block* block : asm_blocks)
        {
            block_il_ptr translated_block = translate_block(block);
            translated_blocks.push_back(translated_block);
        }

        return translated_blocks;
    }

    block_il_ptr ir_translator::translate_block(dasm::basic_block* bb)
    {
        if (bb->decoded_insts.empty())
            return { };

        // todo: separate into 3 blocks (enter, body, exit)
        // so that ot her blocks can just jump to body rather exit and reenter the vm

        const block_il_ptr body = bb_map[bb];
        body->add_command(std::make_shared<cmd_enter>());

        // we calculate skips here because a basic block might end with a jump
        // we will handle that manually instead of letting the il translator handle this
        const dasm::block_end_reason end_reason = bb->get_end_reason();
        const uint8_t skips = end_reason == dasm::block_end ? 0 : 1;

        for (uint32_t i = 0; i < bb->decoded_insts.size() - skips; i++)
        {
            // use il x86 translator to translate the instruction to il
            auto decoded_inst = bb->decoded_insts[i];
            auto& [inst, ops] = decoded_inst;

            std::vector<handler_op> il_operands;
            handler::base_handler_gen_ptr handler_gen = nullptr;
            handler::handler_info_ptr target_handler = nullptr;

            codec::mnemonic mnemonic = static_cast<codec::mnemonic>(inst.mnemonic);
            if (instruction_handlers.contains(mnemonic))
            {
                // first we verify if there is even a valid handler for this inustruction
                // we do this by checking the handler generator for this specific handler
                handler_gen = instruction_handlers[mnemonic];

                for (int j = 0; j < inst.operand_count_visible; j++)
                {
                    il_operands.push_back({
                        static_cast<codec::op_type>(ops[i].type),
                        static_cast<codec::reg_size>(ops[i].size)
                    });
                }

                target_handler = handler_gen->get_operand_handler(il_operands);
            }

            bool translate_sucess = target_handler != nullptr;
            if (target_handler)
            {
                // we know that a valid handler exists for this instruction
                // this means that we can bring it into the base x86 lifter

                // keep track of used handlers
                handler::gen_info_pair info_pair = { handler_gen, target_handler };
                if (!handler_refs.contains(info_pair))
                    handler_refs.emplace(info_pair);

                // now we need to find a lifter
                const uint64_t current_rva = bb->get_index_rva(i);

                auto create_lifter = instruction_lifters[mnemonic];
                const std::shared_ptr<lifter::base_x86_lifter> lifter = create_lifter(decoded_inst, current_rva);

                translate_sucess = lifter->translate_to_il(current_rva);
                if (translate_sucess)
                {
                    // append basic block
                    block_il_ptr result_block = lifter->get_block();

                    // TODO: add way to scatter blocks instead of appending them to a single block
                    // this should probably be done post gen though
                    body->copy_from(result_block);
                }
            }

            if (!translate_sucess)
            {
                // handler does not exist
                // we need to execute the original instruction
                body->add_command(std::make_shared<cmd_x86_exec>(decoded_inst));
            }
        }

        std::vector<il_exit_result> exits;
        exit_condition condition = exit_condition::none;

        switch (end_reason)
        {
            case dasm::block_jump:
            case dasm::block_end:
            {
                auto [target, type] = dasm->get_jump(bb);
                if (type == dasm::jump_outside_segment)
                    exits.push_back(target);
                else
                    exits.push_back(bb_map[dasm->get_block(target)]);

                condition = exit_condition::jump;
                break;
            }
            case dasm::block_conditional_jump:
            {
                // case 1 - condition succeed
                {
                    auto [target, type] = dasm->get_jump(bb);
                    if (type == dasm::jump_outside_segment)
                        exits.push_back(target);
                    else
                        exits.push_back(bb_map[dasm->get_block(target)]);
                }

                // case 2 - fall through
                {
                    auto [target, type] = dasm->get_jump(bb, true);
                    if (type == dasm::jump_outside_segment)
                        exits.push_back(target);
                    else
                        exits.push_back(bb_map[dasm->get_block(target)]);
                }

                condition = exit_condition::conditional;
                break;
            }
        }

        body->add_command(std::make_shared<cmd_exit>(exits, condition));
        return body;
    }

    // needs to be correctly implemented
    std::vector<block_il_ptr> ir_translator::insert_exits(dasm::basic_block* bb, const block_il_ptr& block_base)
    {
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
                    block_base->set_exit(std::make_shared<cmd_exit>(target, exit_condition::jump));
                }
                else
                {
                    // this block exits to another block we are virtualizing
                    block_base->set_exit(std::make_shared<cmd_exit>(bb_map[bb], exit_condition::jump));
                }
            }
            break;
            case dasm::block_conditional_jump:
            {
                // we will have two jump results (first, last)
                std::vector<il_exit_result> results = { };
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

                block_base->set_exit(std::make_shared<cmd_exit>(results, exit_condition::conditional));
            }
            break;
        }
    }

    std::vector<block_il_ptr> ir_translator::optimize(std::vector<block_il_ptr> blocks)
    {
        // the idea of this function is that we want to walk every block
        // for every block, we want to check the block that is being jumped to
        // we want to see if vmenter is ever being used for that block
        // if its not, it can be optimized out and vmexits for blocks that jump to it can be removed as well
        return { };
    }
}
