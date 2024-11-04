#pragma once
#include <unordered_map>

#include "eaglevm-core/codec/zydis_enum.h"
#include "eaglevm-core/compiler/code_label.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_handler_signature.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_type.h"

namespace eagle::virt::eg
{
    struct handler_entry
    {
        std::vector<codec::reg> temp_regs;
        std::vector<codec::reg> virtual_regs;
        std::vector<codec::inst> instructions;

        std::unique_ptr<class asmb::code_label> entry_label;
    };

    class handler_manager
    {
    public:

    private:
        std::unordered_map<ir::command_type, std::vector<handler_entry>> command_handlers;
    };
}
