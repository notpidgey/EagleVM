#pragma once
#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/virtual_machine/ir/models/ir_store.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    using variant_op = std::variant<discrete_store_ptr, reg_vm>;
    class cmd_x86_dynamic : public base_command
    {
    public:
        explicit cmd_x86_dynamic(codec::mnemonic mnemonic, std::initializer_list<variant_op> ops);

        template <typename... Ops>
        explicit cmd_x86_dynamic(const codec::mnemonic mnemonic, Ops... ops)
            : base_command(command_type::vm_exec_dynamic_x86), mnemonic(mnemonic), operands{ ops... }
        {
        }

        codec::mnemonic get_mnemonic() const { return mnemonic; }
        std::vector<variant_op> get_operands() { return operands; }

        bool is_similar(const std::shared_ptr<base_command>& other) override;

    private:
        codec::mnemonic mnemonic;
        std::vector<variant_op> operands;
    };
}
