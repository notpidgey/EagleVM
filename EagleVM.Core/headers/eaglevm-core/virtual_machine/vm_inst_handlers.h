#pragma once
#include <unordered_map>
#include "eaglevm-core/virtual_machine/vm_inst_regs.h"

namespace eagle::virt
{
    class vm_inst_regs;
    namespace handle
    {
        class inst_handler_entry;
        class vm_handler_entry;
    }

    class vm_inst_handlers
    {
    public:
        std::unordered_map<int, handle::inst_handler_entry*> inst_handlers;
        std::unordered_map<int, handle::vm_handler_entry*> v_handlers;

        explicit vm_inst_handlers(vm_inst_regs* push_order);
        void setup_vm_mapping();

    private:
        vm_inst_regs* rm_;
    };
}