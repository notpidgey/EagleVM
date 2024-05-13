#pragma once
#include "eaglevm-core/virtual_machine/machines/pidgeon/vm_inst_regs.h"
#include "eaglevm-core/virtual_machine/machines/pidgeon/vm_inst_handlers.h"

#include "eaglevm-core/virtual_machine/machines/base_machine.h"

namespace eagle::virt::pidg
{
    class machine final : public base_machine
    {
    public:
        machine();

        void call_vm_enter(const asmb::code_label_ptr& container, asmb::code_label_ptr target);
        void call_vm_exit(const asmb::code_label_ptr& container, asmb::code_label_ptr target);
        void create_vm_jump(codec::mnemonic mnemonic, const asmb::code_label_ptr& container, const asmb::code_label_ptr& rva_target);
        codec::encoded_vec create_jump(uint32_t rva, const asmb::code_label_ptr& rva_target);

    private:
        vm_inst_regs_ptr rm_;
        vm_inst_handlers_ptr hg_;
    };
}
