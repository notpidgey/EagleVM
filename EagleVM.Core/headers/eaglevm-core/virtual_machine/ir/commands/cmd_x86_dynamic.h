#pragma once
#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_x86_dynamic : public base_command
    {
    public:
        // TODO: make this a template constructor

        explicit cmd_x86_dynamic(const codec::mnemonic mnemonic, reg_v op1, reg_v op2)
            : base_command(command_type::vm_exec_dynamic_x86)
        {
        }

        explicit cmd_x86_dynamic(const codec::mnemonic mnemonic, reg_v op1)
            : base_command(command_type::vm_exec_dynamic_x86)
        {
        }

    private:
        codec::enc::req request{ };
    };
}
