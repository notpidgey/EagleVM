#pragma once

namespace eagle::il::handlers
{
    enum handler_vm_id
    {
        vm_none = 0,
        vm_enter = 1,
        vm_exit = 2,
        vm_load_reg = 3,
        vm_store_reg = 4,
        vm_rflags_accept = 5,
        vm_rflags_load = 6,
    };
}