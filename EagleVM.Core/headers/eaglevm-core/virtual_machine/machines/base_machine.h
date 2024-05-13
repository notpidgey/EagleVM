#pragma once
#include "eaglevm-core/disassembler/basic_block.h"
#include "eaglevm-core/virtual_machine/ir/commands/include.h"

namespace eagle::machine
{
    using translate_func = std::function<dasm::basic_block(il::base_command, il::block_il_ptr)>;

    class base_machine
    {
    public:
        std::vector<>


    protected:
        std::unordered_map<il::command_type, translate_func> translators;

        void add_translator(il::command_type, const translate_func&);
        translate_func get_translator(il::command_type);
    };
}