#pragma once
#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    using variant_op = std::variant<reg_vm, discrete_store_ptr>;
    class cmd_x86_dynamic : public base_command
    {
    public:
        // TODO: make this a template constructor

        explicit cmd_x86_dynamic(const codec::mnemonic mnemonic, const variant_op& op1, const variant_op& op2)
            : base_command(command_type::vm_exec_dynamic_x86), mnemonic(mnemonic)
        {
            operands.push_back(op1);
            operands.push_back(op2);
        }

        explicit cmd_x86_dynamic(const codec::mnemonic mnemonic, const variant_op& op1)
            : base_command(command_type::vm_exec_dynamic_x86), mnemonic(mnemonic)
        {
            operands.push_back(op1);
        }

        codec::mnemonic get_mnemonic() const
        {
            return mnemonic;
        }

        std::vector<variant_op> get_operands()
        {
            return operands;
        }

    private:
        codec::mnemonic mnemonic;
        std::vector<variant_op> operands;
    };
}
