#pragma once
#include "eaglevm-core/util/section/section_manager.h"

#include "eaglevm-core/virtual_machine/vm_inst_regs.h"
#include "eaglevm-core/virtual_machine/vm_inst_handlers.h"

class vm_inst
{
public:
    vm_inst();

    void init_reg_order();
    section_manager generate_vm_handlers(bool randomize_handler_position) const;

    vm_inst_regs* get_regs();
    vm_inst_handlers* get_handlers();

private:
    vm_inst_regs* rg_;
    vm_inst_handlers* hg_;
};