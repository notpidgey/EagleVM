#pragma once
#include <set>
#include <unordered_map>
#include "commands/cmd_exit.h"

#include "eaglevm-core/disassembler/basic_block.h"
#include "eaglevm-core/virtual_machine/il/block.h"
#include "eaglevm-core/virtual_machine/il/x86/base_handler_gen.h"

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

        std::vector<block_il_ptr> translate(const std::vector<dasm::basic_block*>& asm_blocks);
        block_il_ptr translate_block(dasm::basic_block* bb);
        std::vector<block_il_ptr> insert_exits(dasm::basic_block* bb, const block_il_ptr& block_base);

        std::vector<block_il_ptr> optimize(std::vector<block_il_ptr> blocks);

    private:
        dasm::segment_dasm* dasm;

        std::set<handler::gen_info_pair> handler_refs;
        std::unordered_map<dasm::basic_block*, block_il_ptr> bb_map;
    };
}