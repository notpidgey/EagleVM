#include <utility>

#include "eaglevm-core/virtual_machine/ir/commands/cmd_handler_call.h"

namespace eagle::ir
{
    cmd_handler_call::cmd_handler_call(const codec::mnemonic mnemonic, x86_operand_sig signature)
        : base_command(command_type::vm_handler_call), mnemonic(mnemonic), operand_sig_init(true), o_sig(std::move(signature))
    {
    }

    cmd_handler_call::cmd_handler_call(const codec::mnemonic mnemonic, handler_sig signataure)
        : base_command(command_type::vm_handler_call), mnemonic(mnemonic), operand_sig_init(false), h_sig(std::move(signataure))
    {
    }

    bool cmd_handler_call::is_operand_sig() const
    {
        return operand_sig_init;
    }

    codec::mnemonic cmd_handler_call::get_mnemonic() const
    {
        return mnemonic;
    }

    x86_operand_sig cmd_handler_call::get_x86_signature()
    {
        return o_sig;
    }

    handler_sig cmd_handler_call::get_handler_signature()
    {
        return h_sig;
    }

    x86_cpu_flag cmd_handler_call::get_relevant_flag() const
    {
        return relevant_flags;
    }

    void cmd_handler_call::set_relevant_flags(const x86_cpu_flag flags)
    {
        relevant_flags = flags;
    }

    bool cmd_handler_call::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_handler_call>(other);
        return base_command::is_similar(other) &&
            is_operand_sig() == cmd->is_operand_sig() &&
            get_mnemonic() == cmd->get_mnemonic() &&
            get_x86_signature() == cmd->get_x86_signature() &&
            get_handler_signature() == cmd->get_handler_signature();
    }
}
