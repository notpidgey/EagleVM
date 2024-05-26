#pragma once
#include <utility>

#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_operand_signature.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_handler_signature.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

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
        explicit cmd_handler_call(codec::mnemonic mnemonic, ir_handler_sig signataure);

        [[nodiscard]] bool is_operand_sig() const;
        x86_operand_sig get_x86_signature();
        ir_handler_sig get_handler_signature();

    private:
        call_type call_type = call_type::none;
        codec::mnemonic mnemonic;

        bool operand_sig_init;

        // operand signature initialized
        // this will case the ir interpreter to look up a handler by this signature
        x86_operand_sig operand_sig;

        // handler signature initialized
        ir_handler_sig handler_sig;
    };
}
