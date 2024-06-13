#include "eaglevm-core/virtual_machine/machines/eagle/machine.h"

#include "eaglevm-core/virtual_machine/machines/eagle/handler_manager.h"
#include "eaglevm-core/virtual_machine/machines/eagle/register_manager.h"
#include "eaglevm-core/virtual_machine/machines/eagle/settings.h"
#include "eaglevm-core/virtual_machine/ir/block.h"

#include "eaglevm-core/virtual_machine/machines/register_context.h"

#include "eaglevm-core/virtual_machine/machines/util.h"

#define VIP         reg_man->get_vm_reg(register_manager::index_vip)
#define VSP         reg_man->get_vm_reg(register_manager::index_vsp)
#define VREGS       reg_man->get_vm_reg(register_manager::index_vregs)
#define VCS         reg_man->get_vm_reg(register_manager::index_vcs)
#define VCSRET      reg_man->get_vm_reg(register_manager::index_vcsret)
#define VBASE       reg_man->get_vm_reg(register_manager::index_vbase)

#define VTEMP       reg_man->get_reg_temp(0)
#define VTEMP2      reg_man->get_reg_temp(1)
#define VTEMPX(x)   reg_man->get_reg_temp(x)

using namespace eagle::codec;

namespace eagle::virt::eg
{
    machine::machine(const settings_ptr& settings_info)
    {
        settings = settings_info;
    }

    machine_ptr machine::create(const settings_ptr& settings_info)
    {
        const std::shared_ptr<machine> instance = std::make_shared<machine>(settings_info);
        const std::shared_ptr<register_manager> reg_man = std::make_shared<register_manager>(settings_info);
        reg_man->init_reg_order();
        reg_man->create_mappings();

        const std::shared_ptr<register_context> reg_ctx = std::make_shared<register_context>(reg_man->get_unreserved_temp());
        const std::shared_ptr<handler_manager> han_man = std::make_shared<handler_manager>(instance, reg_man, reg_ctx, settings_info);

        instance->reg_man = reg_man;
        instance->reg_ctx = reg_ctx;
        instance->han_man = han_man;

        return instance;
    }

    asmb::code_container_ptr machine::lift_block(const ir::block_ptr& block)
    {
        const size_t command_count = block->get_command_count();

        const asmb::code_container_ptr code = asmb::code_container::create("block_begin " + std::to_string(command_count), true);
        han_man->set_working_block(code);

        if (block_context.contains(block))
        {
            const asmb::code_label_ptr label = block_context[block];
            code->bind(label);
        }

        for (size_t i = 0; i < command_count; i++)
        {
            const ir::base_command_ptr command = block->get_command(i);
            handle_cmd(code, command);
        }

        return code;
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_context_load_ptr cmd)
    {
        // we want to load this register onto the stack
        const reg target_reg = cmd->get_reg();
        const reg_size reg_size = get_reg_size(target_reg);

        const ir::discrete_store_ptr storage = ir::discrete_store::create(to_ir_size(reg_size));
        storage->finalize_register(reg_ctx->get_any());

        // load into storage
        han_man->load_register(target_reg, storage);

        // push onto stack
        call_push(block, storage);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_context_store_ptr cmd)
    {
        // we want to store a value into this register from the stack
        const reg target_reg = cmd->get_reg();
        const reg_size reg_size = get_reg_size(target_reg);

        const ir::discrete_store_ptr storage = ir::discrete_store::create(to_ir_size(reg_size));
        storage->finalize_register(reg_ctx->get_any());

        // pop into storage
        call_pop(block, storage);

        // store into target
        han_man->store_register(target_reg, storage);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_branch_ptr cmd)
    {
        auto write_jump = [&](ir::il_exit_result jump, mnemonic mnemonic)
        {
            std::visit([&]<typename exit_type>(exit_type&& arg)
            {
                using T = std::decay_t<exit_type>;
                if constexpr (std::is_same_v<T, ir::vmexit_rva>)
                {
                    const ir::vmexit_rva vmexit_rva = arg;
                    const uint64_t rva = vmexit_rva;

                    block->add(RECOMPILE(encode(mnemonic, ZJMPI(rva))));
                }
                else if constexpr (std::is_same_v<T, ir::block_ptr>)
                {
                    const ir::block_ptr& target = arg;

                    const asmb::code_label_ptr label = get_block_label(target);
                    assert(label != nullptr, "block contains missing context");

                    block->add(RECOMPILE(encode(mnemonic, ZJMPR(label))));
                }
            }, jump);
        };

        switch (cmd->get_condition())
        {
            case ir::exit_condition::jmp:
            {
                const ir::il_exit_result jump = cmd->get_condition_default();
                write_jump(jump, m_jmp);

                break;
            }
            case ir::exit_condition::none:
            {
                assert("invalid exit condition");
                break;
            }
            default:
            {
                // conditional
                const ir::il_exit_result conditional_jump = cmd->get_condition_special();
                write_jump(conditional_jump, to_jump_mnemonic(cmd->get_condition()));

                const ir::il_exit_result jump = cmd->get_condition_default();
                write_jump(jump, m_jmp);
            }
        }
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_handler_call_ptr cmd)
    {
        if (cmd->is_operand_sig())
        {
            const auto sig = cmd->get_x86_signature();
            han_man->call_vm_handler(han_man->get_instruction_handler(cmd->get_mnemonic(), sig));
        }
        else
        {
            const auto sig = cmd->get_handler_signature();
            han_man->call_vm_handler(han_man->get_instruction_handler(cmd->get_mnemonic(), sig));
        }
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_mem_read_ptr cmd)
    {
        ir::ir_size target_size = cmd->get_read_size();
        reg target_temp = reg_ctx->get_any();

        // pop address
        call_pop(block, target_temp);

        // mov temp, [address]
        block->add(encode(m_mov, ZREG(target_temp), ZMEMBD(target_temp, 0, TOB(target_size))));

        // push
        call_push(block, get_bit_version(target_temp, to_reg_size(target_size)));
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_mem_write_ptr cmd)
    {
        const ir::ir_size value_size = cmd->get_value_size();
        const ir::ir_size write_size = cmd->get_write_size();

        const std::vector<reg> temps = reg_ctx->get_any_multiple(2);

        const reg temp_value = get_bit_version(temps[0], to_reg_size(value_size));
        const reg temp_address = temps[1];

        if (cmd->get_is_value_nearest())
        {
            call_pop(block, temp_value);
            call_pop(block, temp_address);
        }
        else
        {
            call_pop(block, temp_address);
            call_pop(block, temp_value);
        }

        block->add(encode(m_mov, ZMEMBD(temp_address, 0, TOB(write_size)), ZREG(temp_value)));
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_pop_ptr cmd)
    {
        if (const ir::discrete_store_ptr store = cmd->get_destination_reg())
        {
            reg_ctx->assign(store);
            call_pop(block, store);
        }
        else
        {
            const reg temp = reg_ctx->get_any();
            call_pop(block, temp);
        }
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_push_ptr cmd)
    {
        // call ia32 handler for push
        switch (cmd->get_push_type())
        {
            case ir::info_type::vm_register:
            {
                const ir::reg_vm store = cmd->get_value_register();
                const reg target_reg = reg_vm_to_register(store);

                call_push(block, target_reg);
                break;
            }
            case ir::info_type::vm_temp_register:
            {
                const ir::discrete_store_ptr store = cmd->get_value_temp_register();
                reg_ctx->assign(store);

                call_push(block, store);
                break;
            }
            case ir::info_type::immediate:
            {
                const reg temp_reg = reg_ctx->get_any();
                block->add(encode(m_mov, ZREG(temp_reg), ZIMMU(cmd->get_value_immediate())));

                call_push(block, get_bit_version(temp_reg, to_reg_size(cmd->get_size())));
                break;
            }
            case ir::info_type::address:
            {
                const uint64_t constant = cmd->get_value_immediate();
                const reg temp_reg = reg_ctx->get_any();

                block->add(encode(m_lea, ZREG(temp_reg), ZMEMBD(VBASE, constant, 8)));
                call_push(block, temp_reg);
                break;
            }
            default:
            {
                assert("reached invalid stack_type for push command");
                break;
            }
        }
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_rflags_load_ptr)
    {
        const asmb::code_label_ptr vm_rflags_load = han_man->get_rlfags_load();
        han_man->call_vm_handler(vm_rflags_load);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_rflags_store_ptr)
    {
        const asmb::code_label_ptr vm_rflags_store = han_man->get_rflags_store();
        han_man->call_vm_handler(vm_rflags_store);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_sx_ptr cmd)
    {
        const ir::ir_size ir_current_size = cmd->get_current();
        reg_size current_size = to_reg_size(ir_current_size);

        const ir::ir_size ir_target_size = cmd->get_target();
        const reg_size target_size = to_reg_size(ir_target_size);

        reg temp_reg = reg_ctx->get_any();
        call_pop(block, get_bit_version(temp_reg, current_size));

        // mov eax/ax/al, VTEMP
        block->add(encode(m_mov,
            ZREG(get_bit_version(rax, current_size)),
            ZREG(get_bit_version(temp_reg, current_size))
        ));

        // keep upgrading the operand until we get to destination size
        while (current_size != target_size)
        {
            // other sizes should not be possible
            switch (current_size)
            {
                case bit_32:
                {
                    block->add(encode(m_cdqe));
                    current_size = bit_64;
                    break;
                }
                case bit_16:
                {
                    block->add(encode(m_cwde));
                    current_size = bit_32;
                    break;
                }
                case bit_8:
                {
                    block->add(encode(m_cbw));
                    current_size = bit_16;
                    break;
                }
                default:
                {
                    assert("unable to handle size translation");
                }
            }
        }


        block->add(encode(m_mov,
            ZREG(get_bit_version(temp_reg, current_size)),
            ZREG(get_bit_version(rax, current_size))
        ));

        call_push(block, get_bit_version(temp_reg, target_size));
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_vm_enter_ptr cmd)
    {
        const asmb::code_label_ptr vm_enter = han_man->get_vm_enter();

        const asmb::code_label_ptr ret = asmb::code_label::create("vmenter_ret target");
        block->add(RECOMPILE(encode(m_push, ZLABEL(ret))));
        block->add(RECOMPILE(encode(m_jmp, ZJMPR(vm_enter))));
        block->bind(ret);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_vm_exit_ptr cmd)
    {
        const asmb::code_label_ptr vm_exit = han_man->get_vm_exit();
        const asmb::code_label_ptr ret = asmb::code_label::create("vmexit_ret target");

        // mov VCSRET, ZLABEL(target)
        block->add(RECOMPILE(encode(m_mov, ZREG(VCSRET), ZLABEL(ret))));

        // lea VRIP, [VBASE + vmexit_address]
        block->add(RECOMPILE(encode(m_lea, ZREG(VIP), ZMEMBD(VBASE, vm_exit->get_address(), 8))));
        block->add(encode(m_jmp, ZREG(VIP)));
        block->bind(ret);
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_x86_dynamic_ptr cmd)
    {
        const mnemonic mnemonic = cmd->get_mnemonic();
        std::vector<ir::variant_op> operands = cmd->get_operands();

        enc::req request = create_encode_request(mnemonic);
        for (ir::variant_op& op : operands)
        {
            std::visit([this]<typename reg_type>(reg_type&& arg)
            {
                if constexpr (std::is_same_v<std::decay_t<reg_type>, ir::discrete_store_ptr>)
                {
                    const ir::discrete_store_ptr& store = arg;
                    reg_ctx->assign(store);
                }
            }, op);

            std::visit([&request]<typename reg_type>(reg_type&& arg)
            {
                using T = std::decay_t<reg_type>;
                if constexpr (std::is_same_v<T, ir::discrete_store_ptr>)
                {
                    const ir::discrete_store_ptr& store = arg;
                    add_op(request, ZREG(store->get_store_register()));
                }
            }, op);
        }

        block->add(request);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_x86_exec_ptr cmd)
    {
        block->add(cmd->get_request());
    }

    std::vector<asmb::code_container_ptr> machine::create_handlers()
    {
        return han_man->build_handlers();
    }

    void machine::handle_cmd(const asmb::code_container_ptr& code, const ir::base_command_ptr& command)
    {
        base_machine::handle_cmd(code, command);
        for (ir::discrete_store_ptr& res : command->get_release_list())
            reg_ctx->release(res);
    }

    void machine::call_push(const asmb::code_container_ptr& block, const ir::discrete_store_ptr& shared)
    {
        reg pushing_register;
        const reg_size size = to_reg_size(shared->get_store_size());

        if (!settings->randomize_working_register)
        {
            pushing_register = han_man->get_push_working_register();

            reg target_returning = get_bit_version(pushing_register, size);
            reg target_store = get_bit_version(shared->get_store_register(), size);
            block->add(encode(m_mov, ZREG(target_returning), ZREG(target_store)));
        }
        else
        {
            pushing_register = shared->get_store_register();
        }

        han_man->call_vm_handler(han_man->get_push(pushing_register, size));
    }

    void machine::call_push(const asmb::code_container_ptr& block, const codec::reg target_reg)
    {
        reg pushing_register;
        const reg_size size = get_reg_size(target_reg);

        if (!settings->randomize_working_register)
        {
            pushing_register = han_man->get_push_working_register();

            reg target_returning = get_bit_version(pushing_register, size);
            reg target_store = get_bit_version(target_reg, size);
            block->add(encode(m_mov, ZREG(target_returning), ZREG(target_store)));
        }
        else
        {
            pushing_register = get_bit_version(target_reg, bit_64);
        }

        han_man->call_vm_handler(han_man->get_push(pushing_register, size));
    }

    void machine::call_pop(const asmb::code_container_ptr& block, const ir::discrete_store_ptr& shared)
    {
        reg pushing_register;
        const reg_size size = to_reg_size(shared->get_store_size());

        if (!settings->randomize_working_register)
        {
            pushing_register = han_man->get_pop_working_register();

            reg target_returning = get_bit_version(pushing_register, size);
            reg target_store = get_bit_version(shared->get_store_register(), size);
            block->add(encode(m_mov, ZREG(target_returning), ZREG(target_store)));
        }
        else
        {
            pushing_register = shared->get_store_register();
        }

        han_man->call_vm_handler(han_man->get_pop(pushing_register, size));
    }

    void machine::call_pop(const asmb::code_container_ptr& block, const codec::reg target_reg)
    {
        reg pushing_register;
        const reg_size size = get_reg_size(target_reg);

        if (!settings->randomize_working_register)
        {
            pushing_register = han_man->get_pop_working_register();

            reg target_returning = get_bit_version(pushing_register, size);
            reg target_store = get_bit_version(target_reg, size);
            block->add(encode(m_mov, ZREG(target_returning), ZREG(target_store)));
        }
        else
        {
            pushing_register = get_bit_version(target_reg, bit_64);
        }

        han_man->call_vm_handler(han_man->get_pop(pushing_register, size));
    }

    reg machine::reg_vm_to_register(const ir::reg_vm store) const
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
                assert("invalid case reached for reg_vm");
            break;
        }

        reg reg = none;
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
                assert("invalid case reached for reg_vm");
            break;
        }

        return get_bit_version(reg, to_reg_size(size));
    }
}
