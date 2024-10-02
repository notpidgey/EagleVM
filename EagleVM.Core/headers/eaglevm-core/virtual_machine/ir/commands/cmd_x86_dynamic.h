#pragma once
#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/virtual_machine/ir/models/ir_store.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/dynamic_encoder/encoder.h"

namespace eagle::ir
{
    using variant_op = std::variant<discrete_store_ptr, reg_vm>;

    class cmd_x86_dynamic : public base_command
    {
    public:
        template <typename... Ops>
        explicit cmd_x86_dynamic(const encoder::encoder& encoder)
            : base_command(command_type::vm_exec_dynamic_x86), encoder(encoder)
        {
        }

        encoder::encoder& get_encoder();
        bool is_similar(const std::shared_ptr<base_command>& other) override;

    private:
        encoder::encoder encoder;
    };

    template <typename... Operands>
    inline std::shared_ptr<cmd_x86_dynamic> make_dyn(codec::mnemonic mnemonic, Operands... ops)
    {
        return std::make_shared<cmd_x86_dynamic>(encoder::encoder{ mnemonic, ops... });
    }
}
