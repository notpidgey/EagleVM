#pragma once
#include <cstdint>
#include <memory>
#include <utility>

#include "eaglevm-core/virtual_machine/il/commands/models/cmd_type.h"
#include "eaglevm-core/virtual_machine/il/models/il_reg.h"

namespace eagle::il
{
#define SHARED_DEFINE(x) \
    class x; \
    using x##_ptr = std::shared_ptr<x>

    SHARED_DEFINE(cmd_context_load);
    SHARED_DEFINE(cmd_context_store);
    SHARED_DEFINE(cmd_enter);
    SHARED_DEFINE(cmd_exit);
    SHARED_DEFINE(cmd_handler_call);
    SHARED_DEFINE(cmd_mem_read);
    SHARED_DEFINE(cmd_mem_write);
    SHARED_DEFINE(cmd_vm_pop);
    SHARED_DEFINE(cmd_vm_push);
    SHARED_DEFINE(cmd_x86_exec);

    class base_command
    {
    public:
        explicit base_command(const command_type command)
            : command(command)
        {
        }

    protected:
        command_type command;
    };

    SHARED_DEFINE(base_command);
    using il_insts = std::vector<base_command_ptr>;

}
