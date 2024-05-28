#pragma once
#include <set>
#include <unordered_map>
#include "commands/cmd_branch.h"

#include "eaglevm-core/disassembler/basic_block.h"
#include "eaglevm-core/virtual_machine/ir/block.h"
#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"

namespace eagle::dasm
{
    class segment_dasm;
}

namespace eagle::ir
{
    class ir_preopt_block;
    using ir_preopt_block_ptr = std::shared_ptr<ir_preopt_block>;
    using ir_preopt_block_vec = std::vector<ir_preopt_block_ptr>;

    using ir_preopt_vm_id = std::pair<ir_preopt_block_ptr, uint32_t>;
    using ir_block_vm_id = std::pair<std::vector<block_il_ptr>, uint32_t>;

    class ir_translator
    {
    public:
        explicit ir_translator(dasm::segment_dasm* seg_dasm);

        std::vector<ir_preopt_block_ptr> translate(bool split);
        std::vector<ir_block_vm_id> flatten(const std::vector<ir_preopt_vm_id>& block_vms);
        std::vector<ir_block_vm_id> optimize(const std::vector<ir_preopt_vm_id>& block_vms);

        dasm::basic_block* map_basic_block(const ir_preopt_block_ptr& preopt_target);
        ir_preopt_block_ptr map_preopt_block(dasm::basic_block* basic_block);

    private:
        dasm::segment_dasm* dasm;

        std::unordered_map<dasm::basic_block*, ir_preopt_block_ptr> bb_map;

        ir_preopt_block_ptr translate_block(dasm::basic_block* bb);
        ir_preopt_block_ptr translate_block_split(dasm::basic_block* bb);

        exit_condition get_exit_condition(codec::mnemonic mnemonic);
    };

    class ir_preopt_block
    {
    public:
        void init();

        block_il_ptr get_entry();
        void clear_entry();

        std::vector<block_il_ptr> get_body();
        block_il_ptr get_exit();

        void add_body(const block_il_ptr& block);

    private:
        block_il_ptr entry;
        std::vector<block_il_ptr> body;
        block_il_ptr exit;
    };
}