#include "eaglevm-core/virtual_machine/machines/pidgeon/machine.h"

#include "eaglevm-core/virtual_machine/ir/block.h"
#include "eaglevm-core/virtual_machine/models/vm_defs.h"

#include "eaglevm-core/virtual_machine/ir/commands/include.h"

namespace eagle::virt::pidg
{
    machine::machine(const bool variant_handlers)
    {
        rm_ = std::make_shared<inst_regs>();
        hg_ = std::make_shared<inst_handlers>(rm_);
        variant_register_handlers = variant_handlers;
    }

    std::vector<asmb::code_container_ptr> machine::create_handlers()
    {
        return hg_->build_handlers();
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_context_load_ptr cmd)
    {
        const codec::reg target_reg = cmd->get_reg();
        auto [displacement, size] = rm_->get_stack_displacement(target_reg);

        block->add(encode(codec::m_mov, ZREG(VTEMP), ZIMMU(displacement)));
        call_handler(block, hg_->get_context_load(size));
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_context_store_ptr cmd)
    {
        const codec::reg target_reg = cmd->get_reg();
        auto [displacement, size] = rm_->get_stack_displacement(target_reg);

        block->add(encode(codec::m_mov, ZREG(VTEMP), ZIMMU(displacement)));
        call_handler(block, hg_->get_context_store(size));
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_branch_ptr cmd)
    {
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

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_mem_write_ptr cmd)
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

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_pop_ptr cmd)
    {
        const ir::discrete_store_ptr reg = cmd->get_destination_reg();
        if (reg)
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

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_push_ptr cmd)
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
    }

    void machine::handle_cmd(asmb::code_container_ptr block, ir::cmd_x86_exec_ptr cmd)
    {
    }

    void machine::create_vm_jump(const codec::mnemonic mnemonic, const asmb::code_container_ptr& container,
        const asmb::code_container_ptr& rva_target)
    {
        container->add(RECOMPILE(codec::encode(mnemonic, ZJMPR(rva_target))));
    }

    codec::encoded_vec machine::create_jump(const uint32_t rva, const asmb::code_container_ptr& rva_target)
    {
        codec::enc::req jmp = encode(codec::m_jmp, ZIMMU(rva_target->get_address() - rva - 5));
        return codec::encode_request(jmp);
    }

    void machine::call_handler(const asmb::code_container_ptr& code, const asmb::code_label_ptr& target)
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
