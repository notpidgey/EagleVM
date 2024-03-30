#pragma once
#include "eaglevm-core/assembler/section_manager.h"
#include "eaglevm-core/assembler/section_manager.h"

#include "eaglevm-core/virtual_machine/vm_inst_regs.h"
#include "eaglevm-core/virtual_machine/vm_inst_handlers.h"

namespace eagle::virt
{
    class vm_inst
    {
    public:
        vm_inst();

        void init_reg_order() const;
        asmbl::section_manager generate_vm_handlers(bool randomize_handler_position) const;

        vm_inst_regs* get_regs() const;
        vm_inst_handlers* get_handlers() const;

    private:
        vm_inst_regs* rg_;
        vm_inst_handlers* hg_;
    };
}