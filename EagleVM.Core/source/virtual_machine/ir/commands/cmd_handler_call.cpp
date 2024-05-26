#include <utility>

#include "eaglevm-core/virtual_machine/ir/commands/cmd_handler_call.h"

namespace eagle::ir
{
    cmd_handler_call::cmd_handler_call(const codec::mnemonic mnemonic, x86_operand_sig signature)
        : base_command(command_type::vm_handler_call), mnemonic(mnemonic), operand_sig_init(true), operand_sig(std::move(signature))
    {
    }

    cmd_handler_call::cmd_handler_call(const codec::mnemonic mnemonic, ir_handler_sig signataure)
        : base_command(command_type::vm_handler_call), mnemonic(mnemonic), operand_sig_init(false), handler_sig(std::move(signataure))
    {
    }

    bool cmd_handler_call::is_operand_sig() const
    {
        return operand_sig_init;
    }

    x86_operand_sig cmd_handler_call::get_x86_signature()
    {
        return operand_sig;
    }

    ir_handler_sig cmd_handler_call::get_handler_signature()
    {
        return handler_sig;
    }
}
