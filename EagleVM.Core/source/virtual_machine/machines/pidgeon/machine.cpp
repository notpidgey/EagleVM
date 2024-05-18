#include "eaglevm-core/virtual_machine/machines/pidgeon/machine.h"

#include "eaglevm-core/virtual_machine/ir/block.h"
#include "eaglevm-core/virtual_machine/models/vm_defs.h"

#include "eaglevm-core/virtual_machine/ir/commands/include.h"

namespace eagle::virt::pidg
{
    machine::machine()
    {
        rm_ = std::make_shared<inst_regs>();
        hg_ = std::make_shared<inst_handlers>(rm_);
    }

    std::vector<asmb::code_container_ptr> machine::create_handlers()
    {
        return hg_->build_handlers();
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_context_load_ptr cmd)
    {
        const codec::reg target_reg = cmd->get_reg();
        auto [displacement, size] = rm_->get_stack_displacement(target_reg);

        block->add(encode(codec::m_mov, ZREG(VTEMP), ZIMMU(displacement)));
        call_handler(block, hg_->get_context_load(size));
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_context_store_ptr cmd)
    {
        const codec::reg target_reg = cmd->get_reg();
        auto [displacement, size] = rm_->get_stack_displacement(target_reg);

        block->add(encode(codec::m_mov, ZREG(VTEMP), ZIMMU(displacement)));
        call_handler(block, hg_->get_context_store(size));
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_branch_ptr cmd)
    {
        auto write_jump = [&](ir::il_exit_result jump, codec::mnemonic mnemonic)
        {
            std::visit([&]<typename exit_type>(exit_type&& arg)
            {
                using T = std::decay_t<exit_type>;
                if constexpr (std::is_same_v<T, ir::vmexit_rva>)
                {
                    const ir::vmexit_rva rva = arg;
                    block->add(encode(codec::m_mov, ZREG(VTEMP), ZIMMS(rva)));
                }
                else if constexpr (std::is_same_v<T, ir::block_il_ptr>)
                {
                    const ir::block_il_ptr target = arg;
                    block->add(RECOMPILE(codec::encode(codec::m_mov, ZREG(VTEMP), ZIMMS(target->get))));
                }
            }, jump);

            block->add(encode(mnemonic, ZREG(VTEMP)));
        };

        switch (cmd->get_condition())
        {
            case ir::exit_condition::jmp:
            {
                const ir::il_exit_result jump = cmd->get_condition_default();
                write_jump(jump, codec::m_jmp);
            }
            case ir::exit_condition::none:
            {
                assert("invalid exit condition");
            }
            default:
            {
                // conditional
                const ir::il_exit_result conditional_jump = cmd->get_condition_special();
                write_jump(conditional_jump, to_jump_mnemonic(cmd->get_condition()));
            }
        }
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_handler_call_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_mem_read_ptr cmd)
    {
        // todo: add pop/push variants in handler_data so that we can generate handlers with random pop/push registers
        // todo: inline but also add option to use mov handler
        ir::ir_size target_size = cmd->get_read_size();

        // pop address
        call_handler(block, hg_->get_instruction_handler(codec::m_pop, 1, codec::reg_size::bit_64));

        // mov temp, [address]
        block->add(encode(codec::m_mov, ZREG(VTEMP), ZMEMBD(VTEMP, 0, target_size)));

        // push
        call_handler(block, hg_->get_instruction_handler(codec::m_push, 1, to_reg_size(target_size)));
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_mem_write_ptr cmd)
    {
        // todo: add pop/push variants in handler_data so that we can generate handlers with random pop/push registers
        // todo: il_size should have a reg_size translator somewhere

        ir::ir_size value_size = cmd->get_value_size();
        ir::ir_size write_size = cmd->get_write_size();

        // todo: !!!!! change order of these, value is on top
        // pop vtemp2 ; pop address into vtemp2
        call_handler(block, hg_->get_instruction_handler(codec::m_pop, 1, codec::reg_size::bit_64));
        block->add(encode(codec::m_mov, ZREG(VTEMP2), ZREG(VTEMP)));

        // pop vtemp ; pop value into vtemp
        call_handler(block, hg_->get_instruction_handler(codec::m_pop, 1, to_reg_size(value_size)));

        // mov [vtemp2], vtemp
        codec::reg target_vtemp = get_bit_version(VTEMP, get_gpr_class_from_size(to_reg_size(value_size)));
        block->add(encode(codec::m_mov, ZMEMBD(VTEMP2, 0, write_size), ZREG(target_vtemp)));
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_pop_ptr cmd)
    {
        if (const ir::discrete_store_ptr reg = cmd->get_destination_reg())
        {
            const ir::ir_size pop_size = reg->get_store_size();

            // todo: there needs to be some kind of system for cmd's to contain a do not modify list of discrete_store_ptr
            // movs like this will be destructive

            // call ia32 handler for pop
            call_handler(block, hg_->get_instruction_handler(codec::m_pop, 1, to_reg_size(pop_size)));

            // mov target, vtemp
            codec::reg target_reg = reg->get_store_register();

            // if the target register is VTEMP we do not need to mov
            if (get_bit_version(target_reg, codec::reg_class::gpr_64) != VTEMP)
            {
                codec::reg target_temp = get_bit_version(VTEMP, get_reg_class(target_reg));
                block->add(encode(codec::m_mov, target_reg, target_temp));
            }
        }
        else
        {
            // todo: add pops just by changing rsp instead of calling handler
            const ir::ir_size pop_size = cmd->get_size();
            call_handler(block, hg_->get_instruction_handler(codec::m_pop, 1, to_reg_size(pop_size)));
        }
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_push_ptr cmd)
    {
        // call ia32 handler for push
        switch (cmd->get_push_type())
        {
            case ir::stack_type::vm_register:
            {
                const ir::discrete_store_ptr reg = cmd->get_value_register();
                const codec::reg target_reg = reg->get_store_register();
                const codec::reg_class target_class = get_reg_class(target_reg);

                const codec::reg bit_64_reg = get_bit_version(target_reg, codec::gpr_64);
                if (VTEMP != bit_64_reg)
                {
                    codec::reg target_temp = get_bit_version(VTEMP, target_class);
                    block->add(encode(codec::m_mov, ZREG(target_temp), ZREG(target_reg)));
                }

                const codec::reg_size target_size = get_reg_size(target_reg);
                call_handler(block, hg_->get_instruction_handler(codec::m_push, 1, target_size));
                break;
            }
            case ir::stack_type::immediate:
            {
                const uint64_t constant = cmd->get_value_immediate();
                const ir::ir_size size = cmd->get_value_immediate_size();

                const codec::reg_class target_size = get_gpr_class_from_size(to_reg_size(size));
                const codec::reg target_temp = get_bit_version(VTEMP, target_size);

                block->add(encode(codec::m_mov, ZREG(target_temp), ZIMMU(constant)));
                call_handler(block, hg_->get_instruction_handler(codec::m_push, 1, to_reg_size(size)));
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
        const asmb::code_label_ptr vm_rflags_load = hg_->get_rlfags_load();
        call_handler(block, vm_rflags_load);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_rflags_store_ptr)
    {
        const asmb::code_label_ptr vm_rflags_store = hg_->get_rflags_store();
        call_handler(block, vm_rflags_store);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_sx_ptr cmd)
    {
        const ir::ir_size ir_current_size = cmd->get_current();
        codec::reg_size current_size = to_reg_size(ir_current_size);

        const ir::ir_size ir_target_size = cmd->get_target();
        codec::reg_size target_size = to_reg_size(ir_target_size);

        call_handler(block, hg_->get_instruction_handler(codec::m_pop, 1, current_size));

        // mov eax/ax/al, VTEMP
        block->add(encode(codec::m_mov,
            ZREG(codec::get_bit_version(codec::rax, codec::get_gpr_class_from_size(current_size))),
            ZREG(codec::get_bit_version(VTEMP, codec::get_gpr_class_from_size(current_size)))
        ));

        // keep upgrading the operand until we get to destination size
        while (current_size != target_size)
        {
            // other sizes should not be possible
            switch (current_size)
            {
                case codec::reg_size::bit_32:
                {
                    block->add(encode(codec::m_cdqe));
                    current_size = codec::reg_size::bit_64;
                    break;
                }
                case codec::reg_size::bit_16:
                {
                    block->add(encode(codec::m_cwde));
                    current_size = codec::reg_size::bit_32;
                    break;
                }
                case codec::reg_size::bit_8:
                {
                    block->add(encode(codec::m_cbw));
                    current_size = codec::reg_size::bit_16;
                    break;
                }
                default:
                {
                    assert("unable to handle size translation");
                }
            }
        }

        block->add(encode(codec::m_mov,
            ZREG(codec::get_bit_version(VTEMP, codec::get_gpr_class_from_size(target_size))),
            ZREG(codec::get_bit_version(codec::rax, codec::get_gpr_class_from_size(target_size)))
        ));

        call_handler(block, hg_->get_instruction_handler(codec::m_push, 1, target_size));
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_vm_enter_ptr cmd)
    {
        const asmb::code_label_ptr vm_enter = hg_->get_vm_enter();

        const asmb::code_label_ptr ret = asmb::code_label::create();
        block->add(RECOMPILE(codec::encode(codec::m_push, ZLABEL(ret))));
        block->add(RECOMPILE(codec::encode(codec::m_jmp, ZJMPR(vm_enter))));
        block->bind(ret);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_vm_exit_ptr cmd)
    {
        const asmb::code_label_ptr vm_exit = hg_->get_vm_exit();

        // mov VCSRET, ZLABEL(target)
        const asmb::code_label_ptr ret = asmb::code_label::create();
        block->add(RECOMPILE(codec::encode(codec::m_mov, ZREG(VCSRET), ZLABEL(ret))));

        // lea VRIP, [VBASE + vmexit_address]
        block->add(RECOMPILE(codec::encode(codec::m_lea, ZREG(VIP), ZMEMBD(VBASE, vm_exit->get_address(), 8))));
        block->add(encode(codec::m_jmp, ZREG(VIP)));
        block->bind(ret);
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_x86_dynamic_ptr cmd)
    {
        const codec::mnemonic mnemonic = cmd->get_mnemonic();
        std::vector<ir::variant_op> operands = cmd->get_operands();

        codec::enc::req request = create_encode_request(mnemonic);
        for (ir::variant_op& op : operands)
        {
            std::visit([&request]<typename reg_type>(reg_type&& arg)
            {
                using T = std::decay_t<reg_type>;
                if constexpr (std::is_same_v<T, ir::reg_vm>)
                {
                    const ir::reg_vm vm_reg = arg;
                    codec::add_op(request, ZREG(vm_reg));
                }
                else if constexpr (std::is_same_v<T, ir::discrete_store_ptr>)
                {
                    const ir::discrete_store_ptr store = arg;
                    codec::add_op(request, ZREG(store->get_store_register()));
                }
            }, op);
        }

        block->add(request);
    }

    void machine::handle_cmd(const asmb::code_container_ptr block, ir::cmd_x86_exec_ptr cmd)
    {
        block->add(cmd->get_request());
    }

    void machine::call_handler(const asmb::code_container_ptr& code, const asmb::code_label_ptr& target) const
    {
        assert(target != nullptr, "target cannot be an invalid code label");
        assert(code != nullptr, "code cannot be an invalid code label");

        const asmb::code_label_ptr return_label = asmb::code_label::create("caller return");

        // lea VCS, [VCS - 8]       ; allocate space for new return address
        // mov [VCS], code_label    ; place return rva on the stack
        code->add(encode(codec::m_lea, ZREG(VCS), ZMEMBD(VCS, -8, 8)));
        code->add(RECOMPILE(codec::encode(codec::m_mov, ZMEMBD(VCS, 0, 8), ZLABEL(return_label))));

        // lea VIP, [VBASE + VCSRET]  ; add rva to base
        // jmp VIP
        code->add(RECOMPILE(codec::encode(codec::m_lea, ZREG(VIP), ZMEMBD(VBASE, target->get_address(), 8))));
        code->add(encode(codec::m_jmp, ZREG(VIP)));

        // execution after VM handler should end up here
        code->bind(return_label);
    }
}
