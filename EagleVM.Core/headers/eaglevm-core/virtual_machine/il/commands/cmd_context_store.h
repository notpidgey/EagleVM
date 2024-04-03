#pragma once
#include "eaglevm-core/assembler/x86/zydis_defs.h"
#include "eaglevm-core/assembler/x86/zydis_enum.h"
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"

namespace eagle::il
{
    class cmd_context_store : public base_command
    {
    public:
        explicit cmd_context_store(const asmbl::x86::reg dest)
            : base_command(command_type::vm_handler_call), dest(dest)
        {
        }

    private:
        asmbl::x86::reg dest = asmbl::x86::reg::none;
    };
}
