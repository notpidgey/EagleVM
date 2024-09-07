#include "eaglevm-core/virtual_machine/ir/commands/cmd_x86_dynamic.h"

namespace eagle::ir
{
    cmd_x86_dynamic::cmd_x86_dynamic(const codec::mnemonic mnemonic, const variant_op& op1, const variant_op& op2)
        : base_command(command_type::vm_exec_dynamic_x86), mnemonic(mnemonic)
    {
        operands.push_back(op1);
        operands.push_back(op2);
    }

    cmd_x86_dynamic::cmd_x86_dynamic(const codec::mnemonic mnemonic, const variant_op& op1)
        : base_command(command_type::vm_exec_dynamic_x86), mnemonic(mnemonic)
    {
        operands.push_back(op1);
    }

    codec::mnemonic cmd_x86_dynamic::get_mnemonic() const
    {
        return mnemonic;
    }

    std::vector<variant_op> cmd_x86_dynamic::get_operands()
    {
        return operands;
    }

    bool cmd_x86_dynamic::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::static_pointer_cast<cmd_x86_dynamic>(other);

        const auto ops1 = get_operands();
        const auto ops2 = cmd->get_operands();
        if (ops1.size() != ops2.size())
            return false;

        for (int i = 0; i < ops1.size(); i++)
        {
            const bool eq = std::get<discrete_store_ptr>(ops1[i]) ==
                std::get<discrete_store_ptr>(ops2[i]);
            if (!eq)
                return false;
        }

        return base_command::is_similar(other) &&
            get_mnemonic() == cmd->get_mnemonic();
    }
}
