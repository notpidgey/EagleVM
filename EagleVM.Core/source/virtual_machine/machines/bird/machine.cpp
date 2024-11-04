#include "eaglevm-core/virtual_machine/machines/bird/machine.h"
#include "eaglevm-core/virtual_machine/machines/register_context.h"
#include "eaglevm-core/virtual_machine/machines/eagle/register_manager.h"
#include "eaglevm-core/virtual_machine/machines/eagle/settings.h"

#include <unordered_set>

#include "eaglevm-core/virtual_machine/machines/util.h"

#define VIP reg_man->get_vm_reg(register_manager::index_vip)
#define VSP reg_man->get_vm_reg(register_manager::index_vsp)
#define VREGS reg_man->get_vm_reg(register_manager::index_vregs)
#define VCS reg_man->get_vm_reg(register_manager::index_vcs)
#define VCSRET reg_man->get_vm_reg(register_manager::index_vcsret)
#define VBASE reg_man->get_vm_reg(register_manager::index_vbase)
#define VFLAGS reg_man->get_vm_reg(register_manager::index_vflags)

#define VTEMP reg_man->get_reserved_temp(0)
#define VTEMP2 reg_man->get_reserved_temp(1)
#define VTEMPX(x) reg_man->get_reserved_temp(x)

namespace eagle::virt::eg
{
    bird_machine::bird_machine(const settings_ptr& settings_info)
    {
        settings = settings_info;
        out_block = nullptr;
    }

    bird_machine_ptr bird_machine::create(const settings_ptr& settings_info)
    {
        const std::shared_ptr<bird_machine> instance = std::make_shared<bird_machine>(settings_info);
        const std::shared_ptr<register_manager> reg_man = std::make_shared<register_manager>(settings_info);
        reg_man->init_reg_order();
        reg_man->create_mappings();

        const std::shared_ptr<register_context> reg_ctx_64 = std::make_shared<register_context>(reg_man->get_unreserved_temp(), codec::gpr_64);
        const std::shared_ptr<register_context> reg_ctx_128 = std::make_shared<register_context>(reg_man->get_unreserved_temp_xmm(), codec::xmm_128);
        // const std::shared_ptr<handler_manager> han_man = std::make_shared<handler_manager>(instance, reg_man, reg_ctx_64, reg_ctx_128, settings_info);

        instance->reg_man = reg_man;
        instance->reg_64_container = reg_ctx_64;
        instance->reg_128_container = reg_ctx_128;
        // instance->han_man = han_man;

        return instance;
    }

    asmb::code_container_ptr bird_machine::lift_block(const ir::block_ptr& block)
    {
        const size_t command_count = block->size();

        const asmb::code_container_ptr code = asmb::code_container::create("block_begin " + std::to_string(command_count), true);
        if (block_context.contains(block))
        {
            const asmb::code_label_ptr label = block_context[block];
            code->bind(label);
        }

        // walk backwards each command to see the last time each discrete_ptr was used
        // this will tell us when we can free the register
        std::vector<std::vector<ir::discrete_store_ptr>> store_dead(command_count, { });
        std::unordered_set<ir::discrete_store_ptr> discovered_stores;

        // as a fair warning, this analysis only cares about the store usage in the current block
        // so if you are using a store across multiple blocks, you can wish that goodbye because i will not
        // be implementing that because i do not need it
        for (auto i = command_count; i--;)
        {
            auto ref = block->at(i)->get_use_stores();
            for (auto& store : ref)
            {
                if (!discovered_stores.contains(store))
                {
                    store_dead[i].push_back(store);
                    discovered_stores.insert(store);
                }
            }
        }

        for (size_t i = 0; i < command_count; i++)
        {
            const ir::base_command_ptr command = block->at(i);
            dispatch_handle_cmd(code, command);

            for (auto store : store_dead[i])
                reg_64_container->release(store);
        }

        reg_64_container->reset();
        reg_128_container->reset();

        return code;
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_load_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_store_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_branch_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_handler_call_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_mem_read_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_mem_write_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_pop_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_push_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_rflags_load_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_context_rflags_store_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_sx_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_vm_enter_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_vm_exit_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_x86_dynamic_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_x86_exec_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_flags_load_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_jmp_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_and_ptr& cmd)
    {
        handle_generic_logic_cmd(codec::m_and, cmd->get_size(), cmd->get_preserved());
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_or_ptr& cmd)
    {
        handle_generic_logic_cmd(codec::m_or, cmd->get_size(), cmd->get_preserved());
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_xor_ptr& cmd)
    {
        handle_generic_logic_cmd(codec::m_xor, cmd->get_size(), cmd->get_preserved());
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_shl_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_shr_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_add_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_sub_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_cmp_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_resize_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_cnt_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_smul_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_umul_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_abs_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_log2_ptr& cmd)
    {
    }

    void bird_machine::handle_cmd(const asmb::code_container_ptr& block, const ir::cmd_dup_ptr& cmd)
    {
    }

    codec::reg bird_machine::reg_vm_to_register(const ir::reg_vm store) const
    {
        ir::ir_size size = ir::ir_size::none;
        switch (store)
        {
            case ir::reg_vm::vip:
                size = ir::ir_size::bit_64;
                break;
            case ir::reg_vm::vip_32:
                size = ir::ir_size::bit_32;
                break;
            case ir::reg_vm::vip_16:
                size = ir::ir_size::bit_16;
                break;
            case ir::reg_vm::vip_8:
                size = ir::ir_size::bit_8;
                break;
            case ir::reg_vm::vsp:
                size = ir::ir_size::bit_64;
                break;
            case ir::reg_vm::vsp_32:
                size = ir::ir_size::bit_32;
                break;
            case ir::reg_vm::vsp_16:
                size = ir::ir_size::bit_16;
                break;
            case ir::reg_vm::vsp_8:
                size = ir::ir_size::bit_8;
                break;
            case ir::reg_vm::vbase:
                size = ir::ir_size::bit_64;
                break;
            default:
                VM_ASSERT("invalid case reached for reg_vm");
                break;
        }

        codec::reg reg = codec::reg::none;
        switch (store)
        {
            case ir::reg_vm::vip:
            case ir::reg_vm::vip_32:
            case ir::reg_vm::vip_16:
            case ir::reg_vm::vip_8:
                reg = VIP;
                break;
            case ir::reg_vm::vsp:
            case ir::reg_vm::vsp_32:
            case ir::reg_vm::vsp_16:
            case ir::reg_vm::vsp_8:
                reg = VSP;
                break;
            case ir::reg_vm::vbase:
                reg = VBASE;
                break;
            default:
                VM_ASSERT("invalid case reached for reg_vm");
                break;
        }

        return get_bit_version(reg, to_reg_size(size));
    }

    void bird_machine::handle_generic_logic_cmd(const codec::mnemonic command, const ir::ir_size size, const bool preserved)
    {
        scope_register_manager scope = reg_64_container->create_scope();
        auto [r_par1, r_par2] = scope.reserve<2>();
        r_par1 =  get_bit_version(r_par1, get_size(size));
        r_par2 = get_bit_version(r_par2, get_size(size));

        pop_to_register(r_par2);
        pop_to_register(r_par1);

        if (preserved)
        {
            push_register(r_par1);
            push_register(r_par2);
        }

        out_block->add(encode(command, ZREG(r_par1), ZREG(r_par2)));
        push_register(r_par1);
    }

    void bird_machine::pop_to_register(codec::reg reg)
    {
    }

    void bird_machine::push_register(codec::reg reg)
    {
    }

    codec::reg_size bird_machine::get_size(ir::ir_size size)
    {
    }
}
