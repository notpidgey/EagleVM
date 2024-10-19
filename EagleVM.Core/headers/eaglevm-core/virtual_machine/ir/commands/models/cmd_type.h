#pragma once
#include <string>

namespace eagle::ir
{
    enum class command_type
    {
        none,

        vm_enter,
        vm_exit,

        vm_handler_call,
        vm_reg_load,
        vm_reg_store,

        vm_flags_load,

        vm_push,
        vm_pop,

        vm_mem_read,
        vm_mem_write,

        vm_context_load,
        vm_context_store,
        vm_context_rflags_load,
        vm_context_rflags_store,

        vm_exec_x86,
        vm_exec_dynamic_x86,

        vm_sx,

        vm_branch,
        vm_jmp,

        // bitwise
        vm_and,
        vm_or,
        vm_xor,
        vm_shl,
        vm_shr,
        vm_cnt,

        // arith
        vm_add,
        vm_sub,

        // compare
        vm_cmp,
    };

    std::string command_to_string(command_type cmd);
}
