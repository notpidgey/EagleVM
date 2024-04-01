#pragma once
#include <unordered_map>
#include "commands/cmd_exit.h"
#include "eaglevm-core/disassembler/basic_block.h"

namespace eagle::dasm
{
    class segment_dasm;
}

namespace eagle::il
{
    class il_translator
    {
    public:
        il_translator(dasm::segment_dasm* seg_dasm);

        std::vector<il_bb_ptr> translate(const std::vector<dasm::basic_block*>& blocks);
        std::vector<il_bb_ptr> translate(dasm::basic_block* bb);

        std::vector<il_bb_ptr> optimize(std::vector<il_bb_ptr> blocks);

    private:
        dasm::segment_dasm* dasm;
        std::unordered_map<dasm::basic_block*, il_bb_ptr> bb_map;
    };
}