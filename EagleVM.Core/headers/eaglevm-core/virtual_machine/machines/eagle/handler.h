#pragma once
#include <unordered_map>

#include "eaglevm-core/codec/zydis_enum.h"
#include "eaglevm-core/compiler/code_label.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_handler_signature.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_operand_signature.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_type.h"

namespace eagle::virt::eg
{
    class handler_manager
    {
    public:
        static std::vector<ir::base_command_ptr> generate_handler(codec::mnemonic mnemonic, uint64_t handler_sig);
        static std::vector<ir::base_command_ptr> generate_handler(codec::mnemonic mnemonic, const ir::x86_operand_sig& operand_sig);
        static std::vector<ir::base_command_ptr> generate_handler(codec::mnemonic mnemonic, const ir::handler_sig& handler_sig);
    };
}
