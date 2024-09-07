#pragma once
#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    // todo actually add options
    using variant_op = std::variant<discrete_store_ptr>;
    class cmd_x86_dynamic : public base_command
    {
    public:
        explicit cmd_x86_dynamic(codec::mnemonic mnemonic, const variant_op& op1, const variant_op& op2);
        explicit cmd_x86_dynamic(codec::mnemonic mnemonic, const variant_op& op1);

        codec::mnemonic get_mnemonic() const;
        std::vector<variant_op> get_operands();

        bool is_similar(const std::shared_ptr<base_command>& other) override;

    private:
        codec::mnemonic mnemonic;
        std::vector<variant_op> operands;
    };
}
