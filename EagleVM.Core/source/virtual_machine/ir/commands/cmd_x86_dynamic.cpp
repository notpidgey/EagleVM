#include "eaglevm-core/virtual_machine/ir/commands/cmd_x86_dynamic.h"

namespace eagle::ir
{
    encoder::encoder& cmd_x86_dynamic::get_encoder() { return encoder; }

    bool cmd_x86_dynamic::is_similar(const std::shared_ptr<base_command>& other)
    {
        // const auto cmd = std::static_pointer_cast<cmd_x86_dynamic>(other);
        // const auto ops1 = get_operands();
        // const auto ops2 = cmd->get_operands();
        // if (ops1.size() != ops2.size())
        //     return false;
        // for (int i = 0; i < ops1.size(); i++)
        //{
        //     const bool eq = std::get<discrete_store_ptr>(ops1[i]) ==
        //         std::get<discrete_store_ptr>(ops2[i]);
        //     if (!eq)
        //         return false;
        // }
        // return base_command::is_similar(other) &&
        //     get_mnemonic() == cmd->get_mnemonic();

        return false;
    }

    std::vector<discrete_store_ptr> cmd_x86_dynamic::get_use_stores()
    {
        std::vector<discrete_store_ptr> out;
        for (auto op : encoder.get_operands())
        {
            auto check_variant = [&](encoder::val_variant var)
            {
                if (std::holds_alternative<discrete_store_ptr>(var))
                    out.push_back(std::get<discrete_store_ptr>(var));
            };

            switch (op->operand_type)
            {
                case encoder::operand_type::reg:
                {
                    auto reg_op = std::dynamic_pointer_cast<encoder::operand_reg>(op);
                    check_variant(reg_op->get_reg());

                    break;
                }
                case encoder::operand_type::mem:
                {
                    auto reg_mem = std::dynamic_pointer_cast<encoder::operand_mem>(op);
                    check_variant(reg_mem->get_base());
                    check_variant(reg_mem->get_index());

                    break;
                }
            }
        }

        return out;
    }
}
