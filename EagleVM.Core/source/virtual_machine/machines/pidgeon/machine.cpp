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
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_context_load_ptr cmd)
    {
        cmd->
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_context_store_ptr cmd)
    {
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
        ir::il_size target_size = cmd->get_read_size();

        // pop address
        call_handler(label, hg_->get_instruction_handler(codec::m_pop, ir::il_size::bit_64));

        // mov temp, [address]
        label->add(codec::encode(codec::m_mov, ZREG(VTEMP), ZMEMBD(VTEMP, 0, target_size)));

        // push
        call_handler(label, hg_->get_instruction_handler(codec::m_push, target_size));
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_mem_write_ptr cmd)
    {
        // todo: add pop/push variants in handler_data so that we can generate handlers with random pop/push registers
        // todo: il_size should have a reg_size translator somewhere

        ir::il_size value_size = cmd->get_value_size();
        ir::il_size write_size = cmd->get_write_size();

        // pop vtemp2 ; pop address into vtemp2
        call_handler(label, hg_->get_instruction_handler(codec::m_pop, ir::il_size::bit_64));
        label->add(codec::encode(codec::m_mov, ZREG(VTEMP2), ZREG(VTEMP)));

        // pop vtemp ; pop value into vtemp
        call_handler(label, hg_->get_instruction_handler(codec::m_pop, value_size));

        // mov [vtemp2], vtemp
        codec::reg target_vtemp = get_bit_version(VTEMP, get_gpr_class_from_size(static_cast<codec::reg_size>(value_size)));
        label->add(encode(codec::m_mov, ZMEMBD(VTEMP2, 0, write_size), ZREG(target_vtemp)));
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_pop_ptr cmd)
    {
        // call ia32 handler for pop
        const asmb::code_label_ptr pop = hg_->get_instruction_handler(codec::m_pop, );
        call_handler(label, pop);
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_push_ptr cmd)
    {
        // call ia32 handler for push
        const asmb::code_label_ptr push = hg_->get_instruction_handler(codec::m_push, );
        call_handler(label, push);
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
        ir::il_size current_size = cmd->get_current();
        ir::il_size target_size = cmd->get_target();

        // mov eax/ax/al, VTEMP
        label->add(encode(codec::m_mov,
            ZREG(codec::get_bit_version(codec::rax, current_size)),
            ZREG(codec::get_bit_version(VTEMP, current_size))
        ));

        // keep upgrading the operand until we get to destination size
        while(current_size != target_size)
        {
            // other sizes should not be possible
            switch(current_size)
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
        label->add(RECOMPILE(codec::encode(codec::m_mov, ZREG(VCSRET), ZLABEL(target))));

        // lea VRIP, [VBASE + vmexit_address]
        label->add(RECOMPILE(codec::encode(codec::m_lea, ZREG(VIP), ZMEMBD(VBASE, vm_exit->get_address(), 8))));
        label->add(encode(codec::m_jmp, ZREG(VIP)));
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_x86_dynamic_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_container_ptr label, ir::cmd_x86_exec_ptr cmd)
    {
    }

    void machine::create_vm_jump(const codec::mnemonic mnemonic, const asmb::code_container_ptr& container, const asmb::code_container_ptr& rva_target)
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

        asmb::code_label_ptr return_label = asmb::code_label::create("caller return");

        // lea VCS, [VCS - 8]       ; allocate space for new return address
        // mov [VCS], code_label    ; place return rva on the stack
        code->add(codec::encode(codec::m_lea, ZREG(VCS), ZMEMBD(VCS, -8, 8)));
        code->add(RECOMPILE(codec::encode(codec::m_mov, ZMEMBD(VCS, 0, 8), ZLABEL(return_label))));

        // lea VIP, [VBASE + VCSRET]  ; add rva to base
        // jmp VIP
        code->add(RECOMPILE(codec::encode(codec::m_lea, ZREG(VIP), ZMEMBD(VBASE, target->get_address(), 8))));
        code->add(codec::encode(codec::m_jmp, ZREG(VIP)));

        // execution after VM handler should end up here
        code->bind(return_label);

    }
}
