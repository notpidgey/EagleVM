#include "eaglevm-core/virtual_machine/ir/ir_translator.h"

#include "eaglevm-core/virtual_machine/ir/commands/include.h"
#include "eaglevm-core/virtual_machine/ir/block.h"

#include "eaglevm-core/disassembler/disassembler.h"
#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/virtual_machine/ir/x86/handler_data.h"

namespace eagle::ir
{
    ir_translator::ir_translator(dasm::segment_dasm* seg_dasm)
    {
        dasm = seg_dasm;
    }

    std::vector<ir_preopt_block_ptr> ir_translator::translate()
    {
        // we want to initialzie the entire map with bb translates
        for (dasm::basic_block* block : dasm->blocks)
        {
            const ir_preopt_block_ptr vm_il = std::make_shared<ir_preopt_block>();
            vm_il->init();

            bb_map[block] = vm_il;
        }

        std::vector<ir_preopt_block_ptr> result;
        for (dasm::basic_block* block : dasm->blocks)
            result.push_back(translate_block(block));

        //auto similar_seq = [](const std::vector<ir_preopt_block_ptr>& blocks)
        //    -> std::unordered_map<std::string, std::vector<ir_preopt_block_ptr>>
        //{
        //    std::unordered_map<std::string, std::vector<ir_preopt_block_ptr>> sequence_map;
        //    for (const ir_preopt_block_ptr& block : blocks)
        //    {
        //        auto body = block->get_body();
        //        for (int window_size = 2; window_size <= 4; ++window_size)
        //        {
        //            for (int i = 0; i <= body.size() - window_size; ++i)
        //            {
        //                std::string sequence;
        //                for (int j = i; j < i + window_size; ++j)
        //                    sequence += body[j].first->to_string();

        //                sequence_map[sequence].push_back(block);
        //            }
        //        }
        //    }

        //    std::unordered_map<std::string, std::vector<ir_preopt_block_ptr>> similar_sequences;
        //    for (auto& pair : sequence_map)
        //        if (pair.second.size() > 1)
        //            similar_sequences.insert(pair);

        //    return similar_sequences;
        //};

        return result;
    }

    ir_preopt_block_ptr ir_translator::translate_block(dasm::basic_block* bb)
    {
        ir_preopt_block_ptr block_info = bb_map[bb];
        if (bb->decoded_insts.empty())
            return block_info;

        //
        // entry
        //
        const block_il_ptr entry = block_info->get_entry();
        entry->add_command(std::make_shared<cmd_vm_enter>());

        //
        // body
        //
        bool is_vm_block = true;
        block_il_ptr current_block = std::make_shared<block_ir>(false);

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
            std::optional<op_signature> target_handler = std::nullopt;

            codec::mnemonic mnemonic = static_cast<codec::mnemonic>(inst.mnemonic);
            if (instruction_handlers.contains(mnemonic))
            {
                // first we verify if there is even a valid handler for this inustruction
                // we do this by checking the handler generator for this specific handler
                handler_gen = instruction_handlers[mnemonic];

                for (int j = 0; j < inst.operand_count_visible; j++)
                {
                    il_operands.emplace_back(
                        static_cast<codec::op_type>(ops[i].type),
                        static_cast<codec::reg_size>(ops[i].size)
                    );
                }

                target_handler = handler_gen->get_operand_handler(il_operands);
            }

            bool translate_sucess = target_handler != std::nullopt;
            if (target_handler)
            {
                // we know that a valid handler exists for this instruction
                // this means that we can bring it into the base x86 lifter

                // now we need to find a lifter
                const uint64_t current_rva = bb->get_index_rva(i);

                auto create_lifter = instruction_lifters[mnemonic];
                const std::shared_ptr<lifter::base_x86_translator> lifter = create_lifter(decoded_inst, current_rva);

                translate_sucess = lifter->translate_to_il(current_rva);
                if (translate_sucess)
                {
                    // append basic block
                    block_il_ptr result_block = lifter->get_block();

                    // TODO: add way to scatter blocks instead of appending them to a single block
                    // this should probably be done post gen though
                    if (!is_vm_block)
                    {
                        // the current block is a x86 block
                        const block_il_ptr previous = current_block;
                        block_info->add_body(current_block, false);

                        current_block = std::make_shared<block_ir>(false);
                        current_block->add_command(std::make_shared<cmd_vm_enter>());

                        previous->add_command(std::make_shared<cmd_branch>(current_block, exit_condition::jmp));
                        is_vm_block = true;
                    }

                    current_block->copy_from(result_block);
                }
            }

            if (!translate_sucess)
            {
                if (is_vm_block)
                {
                    if (current_block->get_command_count() == 0)
                    {
                        is_vm_block = false;
                    }
                    else
                    {
                        // the current block is a vm block
                        const block_il_ptr previous = current_block;
                        block_info->add_body(current_block, true);

                        current_block = std::make_shared<block_ir>(true);
                        previous->add_command(std::make_shared<cmd_vm_exit>());
                        previous->add_command(std::make_shared<cmd_branch>(current_block, exit_condition::jmp));
                        is_vm_block = false;
                    }
                }

                // handler does not exist
                // we need to execute the original instruction

                // todo: if rip relative, translate
                current_block->add_command(std::make_shared<cmd_x86_exec>(decoded_inst));
            }
        }

        // make sure to add a vmexit
        if (is_vm_block)
        {
            current_block->add_command(std::make_shared<cmd_vm_exit>());
        }

        // jump to exiting block
        const block_il_ptr exit = block_info->get_exit();
        exit->add_command(std::make_shared<cmd_branch>(current_block, exit_condition::jmp));

        block_info->add_body(current_block, is_vm_block);

        //
        // exit
        //
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
                    exits.push_back(bb_map[dasm->get_block(target)]->get_entry());

                condition = exit_condition::jmp;
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
                        exits.push_back(bb_map[dasm->get_block(target)]->get_entry());
                }

                // case 2 - fall through
                {
                    auto [target, type] = dasm->get_jump(bb, true);
                    if (type == dasm::jump_outside_segment)
                        exits.push_back(target);
                    else
                        exits.push_back(bb_map[dasm->get_block(target)]->get_entry());
                }

                const auto& [instruction, _] = bb->decoded_insts.back();
                condition = get_exit_condition(static_cast<codec::mnemonic>(instruction.mnemonic));
                break;
            }
        }

        exit->add_command(std::make_shared<cmd_branch>(exits, condition));
        return block_info;
    }

    std::vector<block_il_ptr> ir_translator::optimize()
    {
        // remove vm enter block if every reference to vm enter block uses the same vm

        // remove vm exit block if every branching block uses the same vm

        // merge blocks together
    }

    dasm::basic_block* ir_translator::map_basic_block(const ir_preopt_block_ptr& preopt_target)
    {
        for(auto& [bb, preopt] : bb_map)
            if(preopt_target == preopt)
                return bb;

        return nullptr;
    }

    ir_preopt_block_ptr ir_translator::map_preopt_block(dasm::basic_block* basic_block)
    {
        return bb_map[basic_block];
    }

    exit_condition ir_translator::get_exit_condition(const codec::mnemonic mnemonic)
    {
        switch (mnemonic)
        {
            case codec::m_jb:
                return exit_condition::jb;
            case codec::m_jbe:
                return exit_condition::jbe;
            case codec::m_jcxz:
                return exit_condition::jcxz;
            case codec::m_jecxz:
                return exit_condition::jecxz;
            case codec::m_jknzd:
                return exit_condition::jknzd;
            case codec::m_jkzd:
                return exit_condition::jkzd;
            case codec::m_jl:
                return exit_condition::jl;
            case codec::m_jle:
                return exit_condition::jle;
            case codec::m_jmp:
                return exit_condition::jmp;
            case codec::m_jnb:
                return exit_condition::jnb;
            case codec::m_jnbe:
                return exit_condition::jnbe;
            case codec::m_jnl:
                return exit_condition::jnl;
            case codec::m_jnle:
                return exit_condition::jnle;
            case codec::m_jno:
                return exit_condition::jno;
            case codec::m_jnp:
                return exit_condition::jnp;
            case codec::m_jns:
                return exit_condition::jns;
            case codec::m_jnz:
                return exit_condition::jnz;
            case codec::m_jo:
                return exit_condition::jo;
            case codec::m_jp:
                return exit_condition::jp;
            case codec::m_jrcxz:
                return exit_condition::jrcxz;
            case codec::m_js:
                return exit_condition::js;
            case codec::m_jz:
                return exit_condition::jz;
            default:
            {
                assert("invalid conditional jump reached");
                return exit_condition::none;
            }
        }
    }

    void ir_preopt_block::init()
    {
        entry = std::make_shared<block_ir>();
        exit = std::make_shared<block_ir>();
    }

    block_il_ptr ir_preopt_block::get_entry()
    {
        return entry;
    }

    std::vector<ir_vm_x86_block> ir_preopt_block::get_body()
    {
        return body;
    }

    block_il_ptr ir_preopt_block::get_exit()
    {
        return exit;
    }

    void ir_preopt_block::add_body(const block_il_ptr& block, bool is_vm)
    {
        const ir_vm_x86_block pair = { block, is_vm };
        body.push_back(pair);
    }
}
