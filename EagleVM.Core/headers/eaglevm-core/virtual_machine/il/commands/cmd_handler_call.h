#pragma once
#include "eaglevm-core/assembler/x86/zydis_defs.h"
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"

namespace eagle::il
{
    enum class call_type
    {
        none,
        inst_handler,
        vm_handler
    };

    class cmd_handler_call : public base_command
    {
    public:
        explicit cmd_handler_call(const call_type type, const asmbl::x86::mnemonic mnemonic,
            const uint8_t op_count, const asmbl::x86::reg_size size)
            : base_command(command_type::vm_handler_call), call_type(type),
            mnemonic(mnemonic), operand_count(op_count), size(size)
        {
        }

    private:
        call_type call_type = call_type::none;
        asmbl::x86::mnemonic mnemonic;

        uint8_t operand_count;
        asmbl::x86::reg_size size;
    };
}
