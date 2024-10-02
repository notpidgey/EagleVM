#include "eaglevm-core/virtual_machine/ir/commands/cmd_x86_dynamic.h"

namespace eagle::ir
{
    encoder::encoder& cmd_x86_dynamic::get_encoder()
    {
        return encoder;
    }

    bool cmd_x86_dynamic::is_similar(const std::shared_ptr<base_command>& other)
    {
        //const auto cmd = std::static_pointer_cast<cmd_x86_dynamic>(other);
        //const auto ops1 = get_operands();
        //const auto ops2 = cmd->get_operands();
        //if (ops1.size() != ops2.size())
        //    return false;
        //for (int i = 0; i < ops1.size(); i++)
        //{
        //    const bool eq = std::get<discrete_store_ptr>(ops1[i]) ==
        //        std::get<discrete_store_ptr>(ops2[i]);
        //    if (!eq)
        //        return false;
        //}
        //return base_command::is_similar(other) &&
        //    get_mnemonic() == cmd->get_mnemonic();

        return false;
    }
}
