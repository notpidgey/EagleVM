#include "eaglevm-core/virtual_machine/machines/base_machine.h"
#include "eaglevm-core/virtual_machine/ir/block.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_rflags_load.h"
#include "eaglevm-core/virtual_machine/ir/commands/cmd_context_rflags_store.h"

namespace eagle::virt
{
    asmb::code_container_ptr base_machine::lift_block(const ir::block_ptr& block)
    {
        const size_t command_count = block->size();
        const asmb::code_container_ptr code = asmb::code_container::create("block_begin " + command_count, true);

        if (block_context.contains(block))
        {
            const asmb::code_label_ptr label = block_context[block];
            code->label(label);
        }

        for (size_t i = 0; i < command_count; i++)
        {
            const ir::base_command_ptr command = block->at(i);
            dispatch_handle_cmd(code, command);
        }

        return code;
    }

    void base_machine::add_block_context(const std::vector<ir::block_ptr>& blocks)
    {
        for (auto& block : blocks)
        {
            if (block_context.contains(block))
                continue;

            block_context[block] = asmb::code_label::create();
        }
    }

    void base_machine::add_block_context(const ir::block_ptr& block)
    {
        if (block_context.contains(block))
            return;

        block_context[block] = asmb::code_label::create();
    }

    void base_machine::add_block_context(const std::vector<std::pair<ir::block_ptr, asmb::code_label_ptr>>& blocks)
    {
        for (auto& [block, label] : blocks)
        {
            if (block_context.contains(block))
            {
                VM_ASSERT("attempted to overwrite existing block");
                continue;
            }

            block_context[block] = label;
        }
    }

    void base_machine::add_block_context(const ir::block_ptr& block, const asmb::code_label_ptr& label)
    {
        if (block_context.contains(block))
        {
            VM_ASSERT("attempted to overwrite existing block");
            return;
        }

        block_context[block] = label;
    }

    void base_machine::add_block_context(std::unordered_map<ir::block_ptr, asmb::code_label_ptr> block_map)
    {
        block_context.insert(block_map.begin(), block_map.end());
    }

    std::vector<std::pair<ir::block_ptr, asmb::code_label_ptr>> base_machine::get_blocks() const
    {
        std::vector<std::pair<ir::block_ptr, asmb::code_label_ptr>> blocks;
        for (const auto& pair : block_context)
            blocks.emplace_back(pair);

        return blocks;
    }

    void base_machine::dispatch_handle_cmd(const asmb::code_container_ptr& code, const ir::base_command_ptr& command)
    {
        switch (command->get_command_type())
        {
            case ir::command_type::vm_enter:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_vm_enter>(command));
                break;
            case ir::command_type::vm_exit:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_vm_exit>(command));
                break;
            case ir::command_type::vm_handler_call:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_handler_call>(command));
                break;
            case ir::command_type::vm_reg_load:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_context_load>(command));
                break;
            case ir::command_type::vm_reg_store:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_context_store>(command));
                break;
            case ir::command_type::vm_push:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_push>(command));
                break;
            case ir::command_type::vm_pop:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_pop>(command));
                break;
            case ir::command_type::vm_mem_read:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_mem_read>(command));
                break;
            case ir::command_type::vm_mem_write:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_mem_write>(command));
                break;
            case ir::command_type::vm_context_load:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_context_load>(command));
                break;
            case ir::command_type::vm_context_store:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_context_store>(command));
                break;
            case ir::command_type::vm_exec_x86:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_x86_exec>(command));
                break;
            case ir::command_type::vm_context_rflags_load:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_context_rflags_load>(command));
                break;
            case ir::command_type::vm_context_rflags_store:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_context_rflags_store>(command));
                break;
            case ir::command_type::vm_sx:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_sx>(command));
                break;
            case ir::command_type::vm_branch:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_branch>(command));
                break;
            case ir::command_type::vm_flags_load:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_flags_load>(command));
                break;
            case ir::command_type::vm_jmp:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_jmp>(command));
                break;
            case ir::command_type::vm_and:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_and>(command));
                break;
            case ir::command_type::vm_or:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_or>(command));
                break;
            case ir::command_type::vm_xor:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_xor>(command));
                break;
            case ir::command_type::vm_shl:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_shl>(command));
                break;
            case ir::command_type::vm_shr:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_shr>(command));
                break;
            case ir::command_type::vm_add:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_add>(command));
                break;
            case ir::command_type::vm_sub:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_sub>(command));
                break;
            case ir::command_type::vm_cmp:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_cmp>(command));
                break;
            case ir::command_type::vm_resize:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_resize>(command));
                break;
            case ir::command_type::vm_cnt:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_cnt>(command));
                break;
            case ir::command_type::vm_smul:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_smul>(command));
                break;
            case ir::command_type::vm_umul:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_umul>(command));
                break;
            case ir::command_type::vm_abs:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_abs>(command));
                break;
            case ir::command_type::vm_log2:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_log2>(command));
                break;
            case ir::command_type::vm_dup:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_dup>(command));
                break;
            case ir::command_type::vm_call:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_call>(command));
                break;
            case ir::command_type::vm_ret:
                handle_cmd(code, std::static_pointer_cast<ir::cmd_ret>(command));
                break;
            case ir::command_type::none:
                VM_ASSERT("unexpected command type generated");
                break;
        }
    }

    codec::mnemonic base_machine::to_jump_mnemonic(const ir::exit_condition condition)
    {
        switch (condition)
        {
            case ir::exit_condition::jb:
                return codec::m_jb;
            case ir::exit_condition::jbe:
                return codec::m_jbe;
            case ir::exit_condition::jcxz:
                return codec::m_jcxz;
            case ir::exit_condition::jecxz:
                return codec::m_jecxz;
            case ir::exit_condition::jl:
                return codec::m_jl;
            case ir::exit_condition::jle:
                return codec::m_jle;
            case ir::exit_condition::jmp:
                return codec::m_jmp;
            case ir::exit_condition::jo:
                return codec::m_jo;
            case ir::exit_condition::jp:
                return codec::m_jp;
            case ir::exit_condition::jrcxz:
                return codec::m_jrcxz;
            case ir::exit_condition::js:
                return codec::m_js;
            default:
            {
                VM_ASSERT("invalid exit condition reached");
                return codec::m_invalid;
            }
        }
    }

    asmb::code_label_ptr base_machine::get_block_label(const ir::block_ptr& block)
    {
        const bool contains = block_context.contains(block);
        VM_ASSERT(contains, "block must contain label");
        if (contains)
            return block_context[block];

        return nullptr;
    }
}
