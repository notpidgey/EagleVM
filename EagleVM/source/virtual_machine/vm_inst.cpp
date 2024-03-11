#include "virtual_machine/vm_inst.h"

vm_inst::vm_inst()
{
    rg_ = new vm_inst_regs();
    hg_ = new vm_inst_handlers();
}

void vm_inst::init_reg_order()
{
    rg_->init_reg_order();
}

section_manager vm_inst::generate_vm_handlers(bool randomize_handler_position) const
{
    section_manager section(randomize_handler_position);
    hg_->setup_vm_mapping();

    for(const auto& handler : hg_->v_handlers | std::views::values)
        handler->initialize_labels();

    for(const auto& handler : hg_->inst_handlers | std::views::values)
        handler->initialize_labels();

    for(const auto& handler : hg_->v_handlers | std::views::values)
    {
        function_container container = handler->construct_handler();
        section.add(container);
    }

    for(const auto& handler : hg_->inst_handlers | std::views::values)
    {
        function_container container = handler->construct_handler();
        section.add(container);
    }

    return section;
}

vm_inst_regs* vm_inst::get_regs()
{
    return rg_;
}

vm_inst_handlers* vm_inst::get_handlers()
{
    return hg_;
}
