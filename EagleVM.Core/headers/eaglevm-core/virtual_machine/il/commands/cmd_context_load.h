#pragma once
#include "eaglevm-core/assembler/x86/zydis_defs.h"
#include "eaglevm-core/assembler/x86/zydis_enum.h"
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"

namespace eagle::il
{
    class cmd_context_load : public base_command
    {
    public:
        explicit cmd_context_load(const asmbl::x86::reg source, const reg_size size)
            : base_command(command_type::vm_handler_call),
            source(source), size(size)
        {
        }

        explicit cmd_context_load(const asmbl::x86::zydis_register source, const reg_size size)
            : base_command(command_type::vm_handler_call),
            source(static_cast<asmbl::x86::reg>(source)), size(size)
        {
        }

    private:
        asmbl::x86::reg source = asmbl::x86::reg::none;
        reg_size size = reg_size::b64;
    };
}
