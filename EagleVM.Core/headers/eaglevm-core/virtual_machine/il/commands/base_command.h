#pragma once
#include <cstdint>
#include <memory>
#include <utility>

#include "eaglevm-core/virtual_machine/il/models/command_type.h"
#include "eaglevm-core/util/zydis_helper.h"

namespace eagle::il
{
#define SHARED_DEFINE(x) \
    class x; \
    using x##_ptr = std::shared_ptr<x>

    SHARED_DEFINE(vm_enter);
    SHARED_DEFINE(vm_exit);
    SHARED_DEFINE(vm_handler_call);
    SHARED_DEFINE(vm_param);

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
}
