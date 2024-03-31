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
        explicit vm_inst_handlers(vm_inst_regs* push_order);
        void setup_vm_mapping();

        handle::inst_handler_entry* get_inst_handler(int handler);
        handle::vm_handler_entry* get_vm_handler(int handler);

    private:
        vm_inst_regs* rm_;

        std::unordered_map<int, handle::inst_handler_entry*> inst_handlers;
        std::unordered_map<int, handle::vm_handler_entry*> v_handlers;
    };
}