#pragma once
#include <unordered_map>
#include "vm_inst_regs.h"

namespace eagle::virt::pidg
{
    namespace handle
    {
        using inst_handler_entry_ptr = std::shared_ptr<class inst_handler_entry>;
        using vm_handler_entry_ptr = std::shared_ptr<class vm_handler_entry>;
    }

    using vm_inst_handlers_ptr = std::shared_ptr<class vm_inst_handlers>;
    class vm_inst_handlers
    {
    public:
        explicit vm_inst_handlers(vm_inst_regs_ptr push_order);
        void setup_vm_mapping();

        handle::inst_handler_entry_ptr get_inst_handler(int handler);
        handle::vm_handler_entry_ptr get_vm_handler(int handler);

    private:
        vm_inst_regs_ptr rm_;

        std::unordered_map<int, handle::inst_handler_entry_ptr> inst_handlers;
        std::unordered_map<int, handle::vm_handler_entry_ptr> v_handlers;
    };
}