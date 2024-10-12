#include "eaglevm-core/virtual_machine/ir/ir_translator.h"

#include <ranges>
#include <unordered_set>

#include "eaglevm-core/virtual_machine/ir/block.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_x86_exec.h"
#include "eaglevm-core/virtual_machine/ir/commands/include.h"
#include "eaglevm-core/virtual_machine/ir/x86/handler_data.h"

#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/disassembler/analysis/liveness.h"
#include "eaglevm-core/disassembler/disassembler.h"

namespace eagle::ir
{
    ir_translator::ir_translator(dasm::segment_dasm_ptr seg_dasm, dasm::analysis::liveness* liveness)
    {
        dasm = seg_dasm;
        dasm_liveness = liveness;
    }

    std::vector<preopt_block_ptr> ir_translator::translate()
    {
        // we want to initialzie the entire map with bb translates
        for (dasm::basic_block_ptr block : dasm->blocks)
        {
            const preopt_block_ptr vm_il = std::make_shared<preopt_block>();
            vm_il->init(block);

            bb_map[block] = vm_il;
        }

        std::vector<preopt_block_ptr> result;
        for (const dasm::basic_block_ptr& block : dasm->blocks)
            result.push_back(translate_block_split(block));

        return result;
    }

    preopt_block_ptr ir_translator::translate_block_split(dasm::basic_block_ptr bb)
    {
        preopt_block_ptr block_info = bb_map[bb];
        if (bb->decoded_insts.empty())
            return block_info;

        const block_ptr entry = block_info->head;
        block_ptr current_block = std::make_shared<block_ir>(unassigned);
        const block_ptr exit = block_info->tail;

        //
        // entry
        //
        entry->push_back(std::make_shared<cmd_vm_enter>());
        entry->push_back(std::make_shared<cmd_branch>(current_block));

        //
        // body
        //

        // we calculate skips here because a basic block might end with a jump
        // we will handle that manually instead of letting the il translator handle this
        const dasm::block_end_reason end_reason = bb->get_end_reason();

        const auto liveness = dasm_liveness->analyze_block(bb);
        for (uint32_t i = 0; i < bb->decoded_insts.size(); i++)
        {
            // use il x86 translator to translate the instruction to il
            auto decoded_inst = bb->decoded_insts[i];
            auto& [inst, ops] = decoded_inst;

            std::vector<handler_op> il_operands;
            handler::base_handler_gen_ptr handler_gen = nullptr;

            std::optional<uint64_t> target_handler = std::nullopt;

            codec::mnemonic mnemonic = static_cast<codec::mnemonic>(inst.mnemonic);
            if (instruction_handlers.contains(mnemonic))
            {
                // first we verify if there is even a valid handler for this inustruction
                // we do this by checking the handler generator for this specific handler

                // prepare the mnemonic incase its a conditional jump so that we can select the correct handler
                // there will be a cleaner way of doing this but the default jcc instruction handler is located under the
                // codec::m_jmp mnemonic which will be the way we select it here
                if (mnemonic == codec::m_jb || mnemonic == codec::m_jbe || mnemonic == codec::m_jcxz || mnemonic == codec::m_jecxz ||
                    mnemonic == codec::m_jknzd || mnemonic == codec::m_jkzd || mnemonic == codec::m_jl || mnemonic == codec::m_jle ||
                    mnemonic == codec::m_jmp || mnemonic == codec::m_jnb || mnemonic == codec::m_jnbe || mnemonic == codec::m_jnl ||
                    mnemonic == codec::m_jnle || mnemonic == codec::m_jno || mnemonic == codec::m_jnp || mnemonic == codec::m_jns ||
                    mnemonic == codec::m_jnz || mnemonic == codec::m_jo || mnemonic == codec::m_jp || mnemonic == codec::m_jrcxz ||
                    mnemonic == codec::m_js || mnemonic == codec::m_jz)
                {
                    mnemonic = codec::m_jmp;
                }

                handler_gen = instruction_handlers[mnemonic];

                for (int j = 0; j < inst.operand_count_visible; j++)
                {
                    auto op = ops[j];
                    il_operands.emplace_back(static_cast<codec::op_type>(op.type), static_cast<codec::reg_size>(op.size));
                }

                target_handler = handler_gen->get_handler_id(il_operands);
            }

            bool translate_success = target_handler != std::nullopt;
            if (target_handler)
            {
                // we know that a valid handler exists for this instruction
                // this means that we can bring it into the base x86 lifter

                // now we need to find a lifter
                const uint64_t current_rva = bb->get_index_rva(i);

                auto create_lifter = instruction_lifters[mnemonic];
                const std::shared_ptr<lifter::base_x86_translator> lifter = create_lifter(shared_from_this(), decoded_inst, current_rva);

                translate_success = lifter->translate_to_il(current_rva);
                if (translate_success)
                {
                    // append basic block
                    block_ptr result_block = lifter->get_block();
                    auto out_liveness = std::get<1>(liveness[i]);
                    auto inst_flags_liveness = dasm_liveness->compute_inst_flags(decoded_inst);

                    auto relevant_out = inst_flags_liveness - out_liveness;
                    auto relevant_out_flags = relevant_out.get_flags();

                    // TODO: add way to scatter blocks instead of appending them to a single block
                    // this should probably be done post gen though
                    if (current_block->get_block_state() == x86_block)
                    {
                        // the current block is an x86 block
                        const block_ptr previous = current_block;
                        block_info->body.push_back(current_block);

                        current_block = std::make_shared<block_ir>(vm_block);
                        current_block->push_back(std::make_shared<cmd_vm_enter>());

                        previous->push_back(std::make_shared<cmd_branch>(current_block));
                    }

                    current_block->set_block_state(vm_block);
                    current_block->copy_from(result_block);
                }
            }

            if (!translate_success)
            {
                if (current_block->get_block_state() == vm_block)
                {
                    // the current block is a vm block
                    const block_ptr previous = current_block;
                    block_info->body.push_back(current_block);

                    current_block = std::make_shared<block_ir>(x86_block);
                    previous->push_back(std::make_shared<cmd_vm_exit>());
                    previous->push_back(std::make_shared<cmd_branch>(current_block));
                }

                // handler does not exist
                // we need to execute the original instruction
                current_block->set_block_state(x86_block);
                handle_block_command(decoded_inst, current_block, bb->get_index_rva(i));
            }
        }

        // every jcc/jmp is garuanteed to get processed if its at the end of the block
        // this means we didn't process any kind of jump in the block, but we always want the block to end with a branch
        if (bb->get_end_reason() == dasm::block_end)
        {
            // add a branch to make sure we account for this case
            // where there's on instruction to virtualize
            il_exit_result target_exit;

            auto [target, type] = dasm->get_jump(bb);
            if (type == dasm::jump_outside_segment)
                target_exit = target;
            else
                target_exit = bb_map[dasm->get_block(target)]->head;

            current_block->push_back(std::make_shared<cmd_branch>(target_exit));
        }

        // at this point "current_block" should contain a branch to the final block, we want to swap this to the exit block at the very end
        ir::cmd_branch_ptr branch_cmd = std::dynamic_pointer_cast<cmd_branch>(current_block->back());
        VM_ASSERT(branch_cmd->get_command_type() == command_type::vm_branch, "final block command must be a branch");

        // we want to jump to the exit block now
        current_block->pop_back();
        current_block->push_back(std::make_shared<cmd_branch>(exit));

        // add as final block_info body
        block_info->body.push_back(current_block);

        //
        // exit
        //
        if (current_block->get_block_state() == vm_block)
            exit->push_back(std::make_shared<cmd_vm_exit>());

        exit->push_back(branch_cmd);

        return block_info;
    }

    std::vector<flat_block_vmid> ir_translator::flatten(std::unordered_map<preopt_block_ptr, uint32_t>& block_vm_ids,
        std::unordered_map<preopt_block_ptr, block_ptr>& block_tracker)
    {
        // for now we just flatten
        std::vector<flat_block_vmid> block_groups;
        for (const auto& [block, vm_id] : block_vm_ids)
        {
            std::vector<block_ptr> block_group;
            auto entry = block->head;
            auto body = block->body;
            auto exit = block->tail;

            if (block_tracker.contains(block))
            {
                // this is a block we want to track
                if (entry == nullptr)
                {
                    // entry point was optimized out, we want to track body
                    const block_ptr first_body = body.front();
                    block_tracker[block] = first_body;
                }
                else
                {
                    // entry point exists so we will track entry
                    block_tracker[block] = entry;
                }
            }

            block_group.push_back(entry);
            block_group.append_range(body);
            block_group.push_back(exit);
            std::erase(block_group, nullptr);

            block_groups.emplace_back(block_group, vm_id);
        }

        return block_groups;
    }

    std::vector<flat_block_vmid> ir_translator::optimize(std::unordered_map<preopt_block_ptr, uint32_t>& block_vm_ids,
        std::unordered_map<preopt_block_ptr, block_ptr>& block_tracker,
        const std::vector<preopt_block_ptr>& extern_call_blocks)
    {
        optimize_heads(block_vm_ids, block_tracker, extern_call_blocks);
        optimize_same_vm(block_vm_ids, block_tracker, extern_call_blocks);

        return flatten(block_vm_ids, block_tracker);
    }

    void ir_translator::optimize_heads(std::unordered_map<preopt_block_ptr, uint32_t>& block_vm_ids,
        std::unordered_map<preopt_block_ptr, block_ptr>& block_tracker,
        const std::vector<preopt_block_ptr>& extern_call_blocks)
    {
        // check each preopt block to see if vmenter head is even necessary
        for (const auto& preopt : block_vm_ids | std::views::keys)
        {
            const auto head = preopt->head;
            const auto first_body = preopt->body.front();

            // head is useless since we dont need to enter
            if (first_body->get_block_state() != vm_block)
                preopt->head = nullptr;

            // move heads to first body block
            if (preopt->head)
            {
                // at this point we determined the head is relevant
                // we can ignore last since its a branch
                for (auto i = preopt->head->size() - 1; i--;)
                    first_body->insert(first_body->begin(), preopt->head->at(i));
            }

            // replace all references to head with the first body
            for (const auto& seek_preopt : block_vm_ids | std::views::keys)
            {
                const auto tail_branch = seek_preopt->tail->get_branch();
                VM_ASSERT(tail_branch, "tail branch cannot be null");

                tail_branch->rewrite_branch(head, preopt->body.front());
            }
        }
    }

    void ir_translator::optimize_same_vm(std::unordered_map<preopt_block_ptr, uint32_t>& block_vm_ids,
        std::unordered_map<preopt_block_ptr, block_ptr>& block_tracker,
        const std::vector<preopt_block_ptr>& extern_call_blocks)
    {
        // remove vm enter block if every reference to vm enter block uses the same vm
        for (const auto& [preopt_block, vm_id] : block_vm_ids)
        {
            const block_ptr search_enter = preopt_block->head;
            if (search_enter == nullptr)
            {
                // this means we removed the head
                // meaning the first body block doesnt use vm enter
                continue;
            }

            const std::vector redirect_body = preopt_block->body;

            bool vm_enter_unremovable = false;
            std::vector<cmd_branch_ptr> search_enter_refs;

            // check if there are any external calls
            // if there are, we cannot remove vm enter
            for (auto& external : extern_call_blocks)
            {
                if (preopt_block == external)
                {
                    vm_enter_unremovable = true;
                    goto UNREMOVABLE;
                }
            }

            // go through each preopt block
            for (const auto& [search_preopt_block, search_vm_id] : block_vm_ids)
            {
                // go through each block
                // the only blocks that can reference our search block are body and exit
                std::vector<block_ptr> search_blocks;
                search_blocks.append_range(search_preopt_block->body);
                search_blocks.push_back(search_preopt_block->tail);

                for (const block_ptr& search_block : search_blocks)
                {
                    const cmd_branch_ptr branch = search_block->get_branch();
                    if (branch->branch_visits(search_enter))
                    {
                        if (search_vm_id != vm_id)
                            vm_enter_unremovable = true;
                        else
                            search_enter_refs.emplace_back(branch);
                    }

                    if (vm_enter_unremovable)
                        goto UNREMOVABLE;
                }
            }

        UNREMOVABLE:
            if (!vm_enter_unremovable)
            {
                // we found that we can replace each branch to the block with a branch to the body
                for (const auto& branch_ref : search_enter_refs)
                {
                    auto rewrite_branch = [&](il_exit_result& exit_result)
                    {
                        if (std::holds_alternative<block_ptr>(exit_result))
                            exit_result = redirect_body.front();
                    };

                    rewrite_branch(branch_ref->get_condition_default());
                    rewrite_branch(branch_ref->get_condition_special());
                }

                preopt_block->head = nullptr;
            }
        }

        // remove vm exit block if every branching block uses the same vm
        for (const auto& [preopt_block, vm_id] : block_vm_ids)
        {
            // check entery of every block
            const block_ptr preopt_exit = preopt_block->tail;
            const cmd_branch_ptr branch = preopt_exit->get_branch();

            // now we want to check if the current vm group has all the exits of this preopt
            auto is_same_vm = [&](const il_exit_result& exit_result)
            {
                if (std::holds_alternative<block_ptr>(exit_result))
                {
                    const block_ptr exit_block = std::get<block_ptr>(exit_result);
                    for (const auto& [search_preopt_block, search_vm_id] : block_vm_ids)
                    {
                        const bool contains = search_preopt_block->head == exit_block || search_preopt_block->tail == exit_block ||
                            std::ranges::find(search_preopt_block->body, exit_block) != search_preopt_block->body.end();

                        if (contains)
                            return search_vm_id == vm_id;
                    }
                }

                // if one of the exits is an rva then we have to exit no matter what : (
                // otherwise the search would fall through to this fail
                return false;
            };

            if (is_same_vm(branch->get_condition_default()) && is_same_vm(branch->get_condition_special()))
            {
                // this means all exits are of the same vm
                const size_t command_count = preopt_exit->size();
                VM_ASSERT(command_count <= 2 && command_count > 0, "preoptimized exit should not have more than 2 obfuscation");

                // this means we should have a vm exit command
                if (command_count == 2)
                    preopt_exit->erase(preopt_exit->begin());
            }
        }
    }

    dasm::basic_block_ptr ir_translator::map_basic_block(const preopt_block_ptr& preopt_target)
    {
        for (auto& [bb, preopt] : bb_map)
            if (preopt_target == preopt)
                return bb;

        return nullptr;
    }

    preopt_block_ptr ir_translator::map_preopt_block(const dasm::basic_block_ptr& basic_block) { return bb_map[basic_block]; }

    std::pair<exit_condition, bool> ir_translator::get_exit_condition(const codec::mnemonic mnemonic)
    {
        switch (mnemonic)
        {
            // Overflow flag (OF)
            case codec::m_jo:
                return { exit_condition::jo, false };
            case codec::m_jno:
                return { exit_condition::jo, true };

            // Sign flag (SF)
            case codec::m_js:
                return { exit_condition::js, false };
            case codec::m_jns:
                return { exit_condition::js, true };

            // Zero flag (ZF)
            case codec::m_jz:
                return { exit_condition::je, false };
            case codec::m_jnz:
                return { exit_condition::je, true };

            // Carry flag (CF)
            case codec::m_jb:
                return { exit_condition::jb, false };
            case codec::m_jnb:
                return { exit_condition::jb, true };

            // Carry or Zero flag (CF or ZF)
            case codec::m_jbe:
                return { exit_condition::jbe, false };
            case codec::m_jnbe:
                return { exit_condition::jbe, true };

            // Sign flag not equal to Overflow flag (SF != OF)
            case codec::m_jl:
                return { exit_condition::jl, false };
            case codec::m_jnl:
                return { exit_condition::jl, true };

            // Zero flag or Sign flag not equal to Overflow flag (ZF or SF != OF)
            case codec::m_jle:
                return { exit_condition::jle, false };
            case codec::m_jnle:
                return { exit_condition::jle, true };

            // Parity flag (PF)
            case codec::m_jp:
                return { exit_condition::jp, false };
            case codec::m_jnp:
                return { exit_condition::jp, true };

            // CX/ECX/RCX register is zero
            case codec::m_jcxz:
                return { exit_condition::jcxz, false };
            case codec::m_jecxz:
                return { exit_condition::jecxz, false };
            case codec::m_jrcxz:
                return { exit_condition::jrcxz, false };

            // Unconditional jump
            case codec::m_jmp:
                return { exit_condition::jmp, false };

            // Conditions not in the enum, might need to be added
            case codec::m_jknzd:
                return { exit_condition::none, false };
            case codec::m_jkzd:
                return { exit_condition::none, false };

            default:
            {
                VM_ASSERT("invalid conditional jump reached");
                return { exit_condition::none, false };
            }
        }
    }

    void ir_translator::handle_block_command(codec::dec::inst_info decoded_inst, const block_ptr& current_block, const uint64_t current_rva)
    {
        if (codec::has_relative_operand(decoded_inst))
        {
            auto [target_address, op_i] = codec::calc_relative_rva(decoded_inst, current_rva);
            current_block->push_back(std::make_shared<cmd_x86_exec>(
                [decoded_inst, target_address, op_i](const uint32_t rva)
                {
                    // Decode instruction to an encode request
                    codec::enc::req encode_request = codec::decode_to_encode(decoded_inst);
                    codec::enc::op& op = encode_request.operands[op_i];

                    // Adjust the operand based on its type
                    switch (op.type)
                    {
                        case ZYDIS_OPERAND_TYPE_MEMORY:
                        {
                            // Adjust memory displacement
                            op.mem.displacement = target_address - rva;
                            break;
                        }
                        case ZYDIS_OPERAND_TYPE_IMMEDIATE:
                        {
                            // Adjust immediate value
                            op.imm.s = target_address - rva;
                            break;
                        }
                        default:
                        {
                            // Break on unexpected operand type
                            __debugbreak();
                        }
                    }

                    return encode_request;
                }));
        }
        else
        {
            current_block->push_back(std::make_shared<cmd_x86_exec>(decoded_inst));
        }
    }

    branch_info ir_translator::get_branch_info(const uint32_t rva)
    {
        branch_info info = { };

        const auto bb = dasm->get_block(rva);
        switch (bb->get_end_reason())
        {
            case dasm::block_jump:
            case dasm::block_end:
            {
                auto [target, type] = dasm->get_jump(bb);
                if (type == dasm::jump_outside_segment)
                    info.fallthrough_branch = target;
                else
                    info.fallthrough_branch = bb_map[dasm->get_block(target)]->head;

                info.exit_condition = exit_condition::jmp;
                info.inverted_condition = false;
                break;
            }
            case dasm::block_conditional_jump:
            {
                // case 1 - condition succeed
                {
                    auto [target, type] = dasm->get_jump(bb);
                    if (type == dasm::jump_outside_segment)
                        info.conditional_branch = target;
                    else
                        info.conditional_branch = bb_map[dasm->get_block(target)]->head;
                }

                // case 2 - fall through
                {
                    auto [target, type] = dasm->get_jump(bb, true);
                    if (type == dasm::jump_outside_segment)
                        info.fallthrough_branch = target;
                    else
                        info.fallthrough_branch = bb_map[dasm->get_block(target)]->head;
                }

                const auto& [instruction, _] = bb->decoded_insts.back();
                auto [condition, invert] = get_exit_condition(static_cast<codec::mnemonic>(instruction.mnemonic));

                info.exit_condition = condition;
                info.inverted_condition = invert;
                break;
            }
        }

        return info;
    }

    void preopt_block::init(const dasm::basic_block_ptr& block)
    {
        head = std::make_shared<block_ir>(vm_block);
        tail = std::make_shared<block_ir>(vm_block);
        original_block = block;
    }
}
