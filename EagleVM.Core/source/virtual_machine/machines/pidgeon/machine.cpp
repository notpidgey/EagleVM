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

    std::vector<asmb::code_label_ptr> machine::create_handlers()
    {
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_context_load_ptr cmd)
    {
        cmd->
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_context_store_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_branch_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_handler_call_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_mem_read_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_mem_write_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_pop_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_push_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_rflags_load_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_rflags_store_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_sx_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_vm_enter_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_vm_exit_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_x86_dynamic_ptr cmd)
    {
    }

    void machine::handle_cmd(asmb::code_label_ptr label, ir::cmd_x86_exec_ptr cmd)
    {
    }

    void machine::call_vm_enter(const asmb::code_label_ptr& container, const asmb::code_label_ptr target)
    {
        const handle::vm_handler_entry* vmenter = vm_inst_->get_handlers()->v_handlers[MNEMONIC_VM_ENTER];
        const auto vmenter_address = vmenter->get_vm_handler_va(bit64);

        container->add(RECOMPILE(codec::encode(codec::m_push, ZLABEL(target))));
        container->add(RECOMPILE(codec::encode(codec::m_jmp, ZJMPR(vmenter_address))));
    }

    void machine::call_vm_exit(const asmb::code_label_ptr& container, const asmb::code_label_ptr target)
    {
        const handle::vm_handler_entry* vmexit = vm_inst_->get_handlers()->v_handlers[MNEMONIC_VM_EXIT];
        const auto vmexit_address = vmexit->get_vm_handler_va(bit64);

        // mov VCSRET, ZLABEL(target)
        container->add(RECOMPILE(codec::encode(codec::m_mov, ZREG(VCSRET), ZLABEL(target))));

        // lea VRIP, [VBASE + vmexit_address]
        container->add(RECOMPILE(codec::encode(codec::m_lea, ZREG(VIP), ZMEMBD(VBASE, vmexit_address->get(), 8))));
        container->add(codec::encode(codec::m_jmp, ZREG(VIP)));
    }

    void machine::create_vm_jump(const codec::mnemonic mnemonic, const asmb::code_label_ptr& container, const asmb::code_label_ptr& rva_target)
    {
        container->add(RECOMPILE(codec::encode(mnemonic, ZJMPR(rva_target))));
    }

    codec::encoded_vec machine::create_jump(const uint32_t rva, const asmb::code_label_ptr& rva_target)
    {
        codec::enc::req jmp = encode(codec::m_jmp, ZIMMU(rva_target->get_address() - rva - 5));
        return codec::encode_request(jmp);
    }
}
