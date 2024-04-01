#pragma once
#include <cstdint>
#include <memory>
#include <utility>

#include "eaglevm-core/virtual_machine/il/commands/models/command_type.h"
#include "eaglevm-core/util/zydis_helper.h"

namespace eagle::il
{
#define SHARED_DEFINE(x) \
    class x; \
    using x##_ptr = std::shared_ptr<x>

    SHARED_DEFINE(cmd_enter);
    SHARED_DEFINE(cmd_exit);
    SHARED_DEFINE(cmd_handler_call);
    SHARED_DEFINE(cmd_param);

    class base_command
    {
    public:
        explicit base_command(const command_type command)
            : command(command)
        {
        }

    protected:
        command_type command;

    private:
    };

    SHARED_DEFINE(base_command);
    using il_insts = std::vector<base_command_ptr>;
}
