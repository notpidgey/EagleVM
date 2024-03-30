#include <ranges>

#include "eaglevm-core/virtual_machine/vm_inst.h"

#include "eaglevm-core/virtual_machine/handlers/handler/vm_handler_entry.h"
#include "eaglevm-core/virtual_machine/handlers/handler/inst_handler_entry.h"

namespace eagle::virt
{
    vm_inst::vm_inst()
    {
        rg_ = new vm_inst_regs();
        hg_ = new vm_inst_handlers(rg_);
    }

    void vm_inst::init_reg_order() const
    {
        rg_->init_reg_order();
    }

    asmbl::section_manager vm_inst::generate_vm_handlers(const bool randomize_handler_position) const
    {
        asmbl::section_manager section(randomize_handler_position);
        hg_->setup_vm_mapping();

        for (const auto& handler : hg_->v_handlers | std::views::values)
            handler->initialize_labels();

        for (const auto& handler : hg_->inst_handlers | std::views::values)
            handler->initialize_labels();

        for (const auto& handler : hg_->v_handlers | std::views::values)
        {
            asmbl::function_container container = handler->construct_handler();
            section.add(container);
        }

        for (const auto& handler : hg_->inst_handlers | std::views::values)
        {
            asmbl::function_container container = handler->construct_handler();
            section.add(container);
        }

        return section;
    }

    vm_inst_regs* vm_inst::get_regs() const
    {
        return rg_;
    }

    vm_inst_handlers* vm_inst::get_handlers() const
    {
        return hg_;
    }
}
