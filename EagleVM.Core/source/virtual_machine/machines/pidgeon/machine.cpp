#include "eaglevm-core/virtual_machine/machines/pidgeon/machine.h"

#include "eaglevm-core/virtual_machine/ir/block.h"
#include "eaglevm-core/virtual_machine/models/vm_defs.h"

namespace eagle::virt::pidg
{
    machine::machine()
    {
        rm_ = std::make_shared<vm_inst_regs>();
        hg_ = std::make_shared<vm_inst_handlers>(rm_);
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
