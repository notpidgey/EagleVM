#pragma once
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_type.h"
#include "eaglevm-core/virtual_machine/ir/models/il_reg.h"

namespace eagle::ir
{
#define SHARED_DEFINE(x) \
    class x; \
    using x##_ptr = std::shared_ptr<x>

    SHARED_DEFINE(cmd_context_load);
    SHARED_DEFINE(cmd_context_store);
    SHARED_DEFINE(cmd_vm_enter);
    SHARED_DEFINE(cmd_vm_exit);
    SHARED_DEFINE(cmd_handler_call);
    SHARED_DEFINE(cmd_mem_read);
    SHARED_DEFINE(cmd_mem_write);
    SHARED_DEFINE(cmd_pop);
    SHARED_DEFINE(cmd_push);
    SHARED_DEFINE(cmd_rflags_load);
    SHARED_DEFINE(cmd_rflags_store);
    SHARED_DEFINE(cmd_sx);
    SHARED_DEFINE(cmd_x86_dynamic);
    SHARED_DEFINE(cmd_x86_exec);
    SHARED_DEFINE(cmd_branch);

    class base_command
    {
    public:
        explicit base_command(const command_type command)
            : command(command)
        {
        }

        command_type get_command_type();

    protected:
        command_type command;
    };

    SHARED_DEFINE(base_command);
    using ir_insts = std::vector<base_command_ptr>;
}
