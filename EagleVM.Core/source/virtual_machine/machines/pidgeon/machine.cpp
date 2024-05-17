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
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_context_load_ptr cmd)
    {
        const codec::reg target_reg = cmd->get_reg();
        auto [displacement, _] = rm_->get_stack_displacement(target_reg);

        label->add(codec::encode(codec::m_mov, ZREG(VTEMP), ZIMMU(displacement)));
        hg_->get
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_context_store_ptr cmd)
    {
        const codec::reg target_reg = cmd->get_reg();
        auto [displacement, _] = rm_->get_stack_displacement(target_reg);

        label->add(codec::encode(codec::m_mov, ZREG(VTEMP), ZIMMU(displacement)));
        hg_->get
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_branch_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_handler_call_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_mem_read_ptr cmd)
    {
        // todo: add pop/push variants in handler_data so that we can generate handlers with random pop/push registers
        // todo: inline but also add option to use mov handler
        ir::ir_size target_size = cmd->get_read_size();

        // pop address
        call_handler(label, hg_->get_instruction_handler(codec::m_pop, ir::ir_size::bit_64));

        // mov temp, [address]
        label->add(encode(codec::m_mov, ZREG(VTEMP), ZMEMBD(VTEMP, 0, target_size)));

        // push
        call_handler(label, hg_->get_instruction_handler(codec::m_push, target_size));
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_mem_write_ptr cmd)
    {
        // todo: add pop/push variants in handler_data so that we can generate handlers with random pop/push registers
        // todo: il_size should have a reg_size translator somewhere

        ir::ir_size value_size = cmd->get_value_size();
        ir::ir_size write_size = cmd->get_write_size();

        // pop vtemp2 ; pop address into vtemp2
        call_handler(label, hg_->get_instruction_handler(codec::m_pop, ir::ir_size::bit_64));
        label->add(encode(codec::m_mov, ZREG(VTEMP2), ZREG(VTEMP)));

        // pop vtemp ; pop value into vtemp
        call_handler(label, hg_->get_instruction_handler(codec::m_pop, value_size));

        // mov [vtemp2], vtemp
        codec::reg target_vtemp = get_bit_version(VTEMP, get_gpr_class_from_size(static_cast<codec::reg_size>(value_size)));
        label->add(encode(codec::m_mov, ZMEMBD(VTEMP2, 0, write_size), ZREG(target_vtemp)));
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_pop_ptr cmd)
    {
        // call ia32 handler for pop
        const ir::discrete_store_ptr reg = cmd->get_destination_reg();
        codec::reg target_reg = reg->get_register();

        // todo: there needs to be some kind of system for cmd's to contain a do not modify list of discrete_store_ptr
        // movs like this will be destructive
        call_handler(label, hg_->get_instruction_handler(codec::m_pop, ));
        label->add(codec::encode(codec::m_mov, ))
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_push_ptr cmd)
    {
        // call ia32 handler for push
        switch (cmd->get_push_type())
        {
            case ir::stack_type::vm_register:
            {
                const ir::discrete_store_ptr reg = cmd->get_value_register();
                const codec::reg target_reg = reg->get_register();

                const codec::reg bit_64_reg = get_bit_version(target_reg, codec::gpr_64);
                if(VTEMP != bit_64_reg)
                {
                    const codec::reg_class target_size = get_reg_class(target_reg);
                    codec::reg target_temp = get_bit_version(VTEMP, target_size);

                    label->add(encode(codec::m_mov, ZREG(target_temp), ZREG(target_reg)));
                }

                call_handler(label, hg_->get_instruction_handler(codec::m_push, ));
                break;
            }
            case ir::stack_type::immediate:
            {
                const uint64_t constant = cmd->get_value_immediate();
                const ir::ir_size size = cmd->get_value_immediate_size();

                const codec::reg_class target_size = codec::get_gpr_class_from_size(size);
                const codec::reg target_temp = get_bit_version(VTEMP, target_size);

                label->add(encode(codec::m_mov, ZREG(target_temp), ZIMMU(constant)));
                call_handler(label, hg_->get_instruction_handler(codec::m_push, ));
                break;
            }
            default:
            {
                assert("reached invalid stack_type for push command");
                break;
            }
        }
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_rflags_load_ptr cmd)
    {
        const asmb::code_label_ptr vm_rflags_load = hg_->get_rlfags_load();
        call_handler(label, vm_rflags_load);
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_rflags_store_ptr cmd)
    {
        const asmb::code_label_ptr vm_rflags_store = hg_->get_rflags_store();
        call_handler(label, vm_rflags_store);
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_sx_ptr cmd)
    {
        ir::ir_size current_size = cmd->get_current();
        ir::ir_size target_size = cmd->get_target();

        // mov eax/ax/al, VTEMP
        label->add(encode(codec::m_mov,
            ZREG(codec::get_bit_version(codec::rax, current_size)),
            ZREG(codec::get_bit_version(VTEMP, current_size))
        ));

        // keep upgrading the operand until we get to destination size
        while (current_size != target_size)
        {
            // other sizes should not be possible
            switch (current_size)
            {
                case bit32:
                {
                    label->add(encode(codec::m_cdqe));
                    current_size = bit64;
                    break;
                }
                case bit16:
                {
                    label->add(encode(codec::m_cwde));
                    current_size = bit32;
                    break;
                }
                case bit8:
                {
                    label->add(encode(codec::m_cbw));
                    current_size = bit16;
                    break;
                }
            }
        }

        label->add(encode(codec::m_mov,
            ZREG(codec::get_bit_version(VTEMP, target_size)),
            ZREG(codec::get_bit_version(codec::rax, target_size))
        ));
    }

    void machine::handle_cmd(const asmb::code_container_ptr label, ir::cmd_vm_enter_ptr cmd)
    {
        const asmb::code_label_ptr vm_enter = hg_->get_vm_enter();

        label->add(RECOMPILE(codec::encode(codec::m_push, ZLABEL(target))));
        label->add(RECOMPILE(codec::encode(codec::m_jmp, ZJMPR(vm_enter))));
    }

    void machine::handle_cmd(const asmb::code_container_ptr label, ir::cmd_vm_exit_ptr cmd)
    {
        const asmb::code_label_ptr vm_exit = hg_->get_vm_exit();

        // mov VCSRET, ZLABEL(target)
        const asmb::code_label_ptr ret = asmb::code_label::create();
        label->add(RECOMPILE(codec::encode(codec::m_mov, ZREG(VCSRET), ZLABEL(ret))));

        // lea VRIP, [VBASE + vmexit_address]
        label->add(RECOMPILE(codec::encode(codec::m_lea, ZREG(VIP), ZMEMBD(VBASE, vm_exit->get_address(), 8))));
        label->add(encode(codec::m_jmp, ZREG(VIP)));
        label->bind(ret);
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_x86_dynamic_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_x86_exec_ptr cmd)
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
