#pragma once
#include <utility>

#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_operand_signature.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_handler_signature.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/x86/models/flags.h"

namespace eagle::ir
{
    enum class call_type
    {
        none,
        inst_handler,
    };

    class cmd_handler_call : public base_command
    {
    public:
        explicit cmd_handler_call(codec::mnemonic mnemonic, x86_operand_sig signature);
        explicit cmd_handler_call(codec::mnemonic mnemonic, handler_sig signataure);

        [[nodiscard]] bool is_operand_sig() const;

        codec::mnemonic get_mnemonic() const;
        x86_operand_sig get_x86_signature();
        handler_sig get_handler_signature();

        x86_cpu_flag get_relevant_flag() const;
        void set_relevant_flags(x86_cpu_flag flags);

        bool is_similar(const std::shared_ptr<base_command>& other) override;
        BASE_COMMAND_CLONE(cmd_handler_call);

    private:
        call_type call_type = call_type::none;
        codec::mnemonic mnemonic;

        x86_cpu_flag relevant_flags;
        bool operand_sig_init;

        // operand signature initialized
        // this will case the ir interpreter to look up a handler by this signature
        x86_operand_sig o_sig;

        // handler signature initialized
        handler_sig h_sig;
    };

    SHARED_DEFINE(cmd_handler_call);
}
