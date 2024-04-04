#pragma once
#include "eaglevm-core/compiler/x86/zydis_defs.h"
#include "eaglevm-core/compiler/x86/zydis_enum.h"
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"

namespace eagle::il
{
    class cmd_context_load : public base_command
    {
    public:
        explicit cmd_context_load(const asmb::x86::reg source, const stack_disp size)
            : base_command(command_type::vm_handler_call),
            source(source), size(size)
        {
        }

        explicit cmd_context_load(const asmb::x86::zydis_register source, const stack_disp disp)
            : base_command(command_type::vm_handler_call),
            source(static_cast<asmb::x86::reg>(source)), size(disp)
        {
        }

    private:
        asmb::x86::reg source = asmb::x86::reg::none;
        stack_disp size = stack_disp::bit_64;
    };
}
