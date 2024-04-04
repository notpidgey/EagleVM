#pragma once
#include "eaglevm-core/compiler/x86/zydis_defs.h"
#include "eaglevm-core/compiler/x86/zydis_enum.h"
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"

namespace eagle::il
{
    class cmd_context_store : public base_command
    {
    public:
        explicit cmd_context_store(const asmb::x86::reg dest)
            : base_command(command_type::vm_handler_call), dest(dest)
        {
        }

    private:
        asmb::x86::reg dest = asmb::x86::reg::none;
    };
}
