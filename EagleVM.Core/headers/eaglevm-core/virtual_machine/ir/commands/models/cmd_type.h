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
        vm_carry,

        vm_mem_read,
        vm_mem_write,

        vm_context_load,
        vm_context_store,
        vm_context_rflags_load,
        vm_context_rflags_store,

        vm_exec_x86,

        vm_sx,
        vm_resize,

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
        vm_smul,
        vm_umul,

        vm_abs,
        vm_log2,

        vm_dup,

        // compare
        vm_cmp,

        // control flow
        vm_call,
        vm_ret
    };

    std::string command_to_string(command_type cmd);
}
