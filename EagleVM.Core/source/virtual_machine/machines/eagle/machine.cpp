#include "eaglevm-core/virtual_machine/machines/eagle/machine.h"

#include "eaglevm-core/virtual_machine/machines/eagle/handler_manager.h"
#include "eaglevm-core/virtual_machine/machines/eagle/register_manager.h"
#include "eaglevm-core/virtual_machine/machines/eagle/settings.h"

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
    machine::machine(const machine_settings_ptr& settings_info)
    {
        settings = settings_info;
    }

    machine_ptr machine::create(const machine_settings_ptr& settings_info)
    {
        const std::shared_ptr<machine> instance = std::make_shared<machine>(settings_info);
        instance->reg_man = std::make_shared<register_manager>(settings_info);
        instance->han_man = std::make_shared<handler_manager>(instance, instance->reg_man, settings_info);

        instance->reg_man->init_reg_order();
        instance->reg_ctx = std::make_shared<register_context>(instance->reg_man->get_unreserved_temp());

        return instance;
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
        call_push(storage);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_context_store_ptr cmd)
    {
        // we want to store a value into this register from the stack
        const reg target_reg = cmd->get_reg();
        const reg_size reg_size = get_reg_size(target_reg);

        const ir::discrete_store_ptr storage = ir::discrete_store::create(to_ir_size(reg_size));
        storage->finalize_register(reg_ctx->get_any());

        // pop into storage
        call_pop(storage);

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
        call_pop(target_temp);

        // mov temp, [address]
        block->add(encode(m_mov, ZREG(target_temp), ZMEMBD(target_temp, 0, TOB(target_size))));

        // push
        call_push(get_bit_version(target_temp, to_reg_size(target_size)));
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_mem_write_ptr cmd)
    {
        const ir::ir_size value_size = cmd->get_value_size();
        const ir::ir_size write_size = cmd->get_write_size();

        const std::vector<reg> temps = reg_ctx->get_any_multiple(2);

        const reg temp_value = get_bit_version(temps[0], to_reg_size(value_size));
        const reg temp_address = temps[1];

        if(cmd->get_is_value_nearest())
        {
            call_pop(temp_value);
            call_pop(temp_address);
        }
        else
        {
            call_pop(temp_address);
            call_pop(temp_value);
        }

        block->add(encode(m_mov, ZMEMBD(temp_address, 0, TOB(write_size)), ZREG(temp_value)));
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_pop_ptr cmd)
    {
        if (const ir::discrete_store_ptr store = cmd->get_destination_reg())
        {
            reg_ctx->assign(store);
            call_pop(store);
        }
        else
        {
            const reg temp = reg_ctx->get_any();
            call_pop(temp);
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

                call_push(target_reg);
                break;
            }
            case ir::info_type::vm_temp_register:
            {
                const ir::discrete_store_ptr store = cmd->get_value_temp_register();
                reg_ctx->assign(store);

                call_push(store);
                break;
            }
            case ir::info_type::immediate:
            {
                const reg temp_reg = reg_ctx->get_any();
                block->add(encode(m_mov, temp_reg, cmd->get_value_immediate()));

                call_push(get_bit_version(temp_reg, to_reg_size(cmd->get_size())));
                break;
            }
            case ir::info_type::address:
            {
                const uint64_t constant = cmd->get_value_immediate();
                const reg temp_reg = reg_ctx->get_any();

                block->add(encode(m_lea, ZREG(temp_reg), ZMEMBD(VBASE, constant, 8)));
                call_push(temp_reg);
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
        call_pop(temp_reg, current_size);

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

        call_push(temp_reg, target_size);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_vm_enter_ptr cmd)
    {
        const asmb::code_label_ptr vm_enter = han_man->get_vm_enter();

        const asmb::code_label_ptr ret = asmb::code_label::create();
        block->add(RECOMPILE(encode(m_push, ZLABEL(ret))));
        block->add(RECOMPILE(encode(m_jmp, ZJMPR(vm_enter))));
        block->bind(ret);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_vm_exit_ptr cmd)
    {
        const asmb::code_label_ptr vm_exit = han_man->get_vm_exit();
        const asmb::code_label_ptr ret = asmb::code_label::create();

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
    }

    void machine::handle_cmd(const asmb::code_container_ptr& code, const ir::base_command_ptr& command)
    {
        base_machine::handle_cmd(code, command);
        for (ir::discrete_store_ptr& res : command->get_release_list())
            reg_ctx->release(res);
    }

    void machine::call_push(const ir::discrete_store_ptr& shared)
    {
        reg returning_register = han_man->call_push(sha)
        if(settings->randomize_working_register)
        {


        }
        else
        {

        }
    }

    void machine::call_push(const codec::reg reg)
    {
    }

    void machine::call_push(const codec::reg reg, codec::reg_size target)
    {
    }

    void machine::call_pop(const ir::discrete_store_ptr& shared)
    {
    }

    void machine::call_pop(const codec::reg reg)
    {

    }

    void machine::call_pop(const codec::reg reg, codec::reg_size target)
    {
    }
}
