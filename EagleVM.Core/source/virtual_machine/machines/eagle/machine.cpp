#include "eaglevm-core/virtual_machine/machines/eagle/machine.h"

#include "eaglevm-core/virtual_machine/machines/eagle/inst_handlers.h"
#include "eaglevm-core/virtual_machine/machines/eagle/inst_regs.h"
#include "eaglevm-core/virtual_machine/machines/eagle/settings.h"

#include "eaglevm-core/virtual_machine/machines/util.h"

#define VIP         rm->get_reg(inst_regs::index_vip)
#define VSP         rm->get_reg(inst_regs::index_vsp)
#define VREGS       rm->get_reg(inst_regs::index_vregs)
#define VCS         rm->get_reg(inst_regs::index_vcs)
#define VCSRET      rm->get_reg(inst_regs::index_vcsret)
#define VBASE       rm->get_reg(inst_regs::index_vbase)

#define VTEMP       rm->get_reg_temp(0)
#define VTEMP2      rm->get_reg_temp(1)
#define VTEMPX(x)   rm->get_reg_temp(x)

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
        instance->rm = std::make_shared<inst_regs>(settings_info);
        instance->handlers = std::make_shared<inst_handlers>(instance, instance->rm, settings_info);
        instance->rm->init_reg_order();

        instance->reg_man = std::make_shared<transaction_handler>(instance->rm->get_availiable_temp());

        return instance;
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_context_load_ptr cmd)
    {
        // we want to load this register onto the stack
        const reg target_reg = cmd->get_reg();
        const reg_size reg_size = get_reg_size(target_reg);

        const ir::discrete_store_ptr storage = ir::discrete_store::create(to_ir_size(reg_size));
        storage->finalize_register(reg_man->get_any());

        // load into storage
        handlers->load_register(target_reg, storage);

        // push onto stack
        handlers->push(storage);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_context_store_ptr cmd)
    {
        // we want to store a value into this register from the stack
        const reg target_reg = cmd->get_reg();
        const reg_size reg_size = get_reg_size(target_reg);

        const ir::discrete_store_ptr storage = ir::discrete_store::create(to_ir_size(reg_size));
        storage->finalize_register(reg_man->get_any());

        // pop into storage
        handlers->pop(storage);

        // store into target
        handlers->store_register(target_reg, storage);
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
            handlers->call_vm_handler(handlers->get_instruction_handler(cmd->get_mnemonic(), sig));
        }
        else
        {
            const auto sig = cmd->get_handler_signature();
            handlers->call_vm_handler(handlers->get_instruction_handler(cmd->get_mnemonic(), sig));
        }
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_mem_read_ptr cmd)
    {
        ir::ir_size target_size = cmd->get_read_size();
        reg target_temp = reg_man->get_any();

        // pop address
        handlers->pop(target_temp);

        // mov temp, [address]
        block->add(encode(m_mov, ZREG(target_temp), ZMEMBD(target_temp, 0, TOB(target_size))));

        // push
        handlers->push(get_bit_version(target_temp, to_reg_size(target_size)));
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_mem_write_ptr cmd)
    {
        const ir::ir_size value_size = cmd->get_value_size();
        const ir::ir_size write_size = cmd->get_write_size();

        const std::vector<reg> temps = reg_man->get_any_multiple(2);

        const reg temp_value = get_bit_version(temps[0], to_reg_size(value_size));
        const reg temp_address = temps[1];

        if(cmd->get_is_value_nearest())
        {
            handlers->pop(temp_value);
            handlers->pop(temp_address);
        }
        else
        {
            handlers->pop(temp_address);
            handlers->pop(temp_value);
        }

        block->add(encode(m_mov, ZMEMBD(temp_address, 0, TOB(write_size)), ZREG(temp_value)));
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_pop_ptr cmd)
    {
        if (const ir::discrete_store_ptr store = cmd->get_destination_reg())
        {
            const ir::ir_size pop_size = store->get_store_size();
            reg_man->assign(store);

            // call ia32 handler for pop
            handlers->call_vm_handler(handlers->get_pop(to_reg_size(pop_size)));

            // mov target, vtemp
            reg target_reg = store->get_store_register();

            // if the target register is VTEMP we do not need to mov
            if (get_bit_version(target_reg, gpr_64) != VTEMP)
            {
                reg target_temp = get_bit_version(VTEMP, get_reg_class(target_reg));
                block->add(encode(m_mov, ZREG(target_reg), ZREG(target_temp)));
            }
        }
        else
        {
            // todo: add pops just by changing rsp instead of calling handler
            const ir::ir_size pop_size = cmd->get_size();
            handlers->call_vm_handler(handlers->get_pop(to_reg_size(pop_size)));
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
                const reg_class target_class = get_reg_class(target_reg);

                const reg bit_64_reg = get_bit_version(target_reg, gpr_64);
                if (VTEMP != bit_64_reg)
                {
                    reg target_temp = get_bit_version(VTEMP, target_class);
                    block->add(encode(m_mov, ZREG(target_temp), ZREG(target_reg)));
                }

                const reg_size target_size = get_reg_size(target_reg);
                handlers->call_vm_handler(handlers->get_push(target_size));

                break;
            }
            case ir::info_type::vm_temp_register:
            {
                const ir::discrete_store_ptr store = cmd->get_value_temp_register();
                if (!store->get_finalized())
                    assign_discrete_storage({ store }, cmd->get_block_list());

                const reg target_reg = store->get_store_register();
                const reg_class target_class = get_reg_class(target_reg);

                const reg bit_64_reg = get_bit_version(target_reg, gpr_64);
                if (VTEMP != bit_64_reg)
                {
                    reg target_temp = get_bit_version(VTEMP, target_class);
                    block->add(encode(m_mov, ZREG(target_temp), ZREG(target_reg)));
                }

                const reg_size target_size = get_reg_size(target_reg);
                handlers->call_vm_handler(handlers->get_push(target_size));

                break;
            }
            case ir::info_type::immediate:
            {
                const uint64_t constant = cmd->get_value_immediate();
                const ir::ir_size size = cmd->get_size();

                const reg_class target_size = get_gpr_class_from_size(to_reg_size(size));
                const reg target_temp = get_bit_version(VTEMP, target_size);

                block->add(encode(m_mov, ZREG(target_temp), ZIMMU(constant)));
                handlers->call_vm_handler(handlers->get_push(to_reg_size(size)));

                break;
            }
            case ir::info_type::address:
            {
                const uint64_t constant = cmd->get_value_immediate();

                block->add(encode(m_lea, ZREG(VTEMP), ZMEMBD(VBASE, constant, 8)));
                handlers->call_vm_handler(handlers->get_push(bit_64));

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
        const asmb::code_label_ptr vm_rflags_load = handlers->get_rlfags_load();
        handlers->call_vm_handler(vm_rflags_load);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_rflags_store_ptr)
    {
        const asmb::code_label_ptr vm_rflags_store = handlers->get_rflags_store();
        handlers->call_vm_handler(vm_rflags_store);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_sx_ptr cmd)
    {
        const ir::ir_size ir_current_size = cmd->get_current();
        reg_size current_size = to_reg_size(ir_current_size);

        const ir::ir_size ir_target_size = cmd->get_target();
        const reg_size target_size = to_reg_size(ir_target_size);

        handlers->call_vm_handler(handlers->get_pop(current_size));

        // mov eax/ax/al, VTEMP
        block->add(encode(m_mov,
            ZREG(get_bit_version(rax, get_gpr_class_from_size(current_size))),
            ZREG(get_bit_version(VTEMP, get_gpr_class_from_size(current_size)))
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
            ZREG(get_bit_version(VTEMP, get_gpr_class_from_size(target_size))),
            ZREG(get_bit_version(rax, get_gpr_class_from_size(target_size)))
        ));

        handlers->call_vm_handler(handlers->get_push(target_size));
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_vm_enter_ptr cmd)
    {
        const asmb::code_label_ptr vm_enter = handlers->get_vm_enter();

        const asmb::code_label_ptr ret = asmb::code_label::create();
        block->add(RECOMPILE(encode(m_push, ZLABEL(ret))));
        block->add(RECOMPILE(encode(m_jmp, ZJMPR(vm_enter))));
        block->bind(ret);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_vm_exit_ptr cmd)
    {
        const asmb::code_label_ptr vm_exit = handlers->get_vm_exit();
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
                    reg_man->assign(store);
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
            reg_man->release(res);
    }
}
