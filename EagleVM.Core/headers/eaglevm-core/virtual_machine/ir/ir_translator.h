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
    class preopt_block;
    using preopt_block_ptr = std::shared_ptr<preopt_block>;
    using preopt_block_vec = std::vector<preopt_block_ptr>;

    using preopt_vm_id = std::pair<preopt_block_ptr, uint32_t>;
    using block_vm_id = std::pair<std::vector<block_ptr>, uint32_t>;

    class ir_translator
    {
    public:
        explicit ir_translator(dasm::segment_dasm* seg_dasm);

        std::vector<preopt_block_ptr> translate(bool split);
        std::vector<block_vm_id> flatten(
            const std::vector<preopt_vm_id>& block_vms,
            std::unordered_map<preopt_block_ptr, block_ptr>& block_tracker
        );
        std::vector<block_vm_id> optimize(
            const std::vector<preopt_vm_id>& block_vms,
            std::unordered_map<preopt_block_ptr, block_ptr>& block_tracker,
            const std::vector<preopt_block_ptr>& extern_call_blocks = { }
        );

        dasm::basic_block* map_basic_block(const preopt_block_ptr& preopt_target);
        preopt_block_ptr map_preopt_block(dasm::basic_block* basic_block);

    private:
        dasm::segment_dasm* dasm;

        std::unordered_map<dasm::basic_block*, preopt_block_ptr> bb_map;

        preopt_block_ptr translate_block(dasm::basic_block* bb);
        preopt_block_ptr translate_block_split(dasm::basic_block* bb);

        exit_condition get_exit_condition(codec::mnemonic mnemonic);

        static void handle_block_command(codec::dec::inst_info decoded_inst, const block_ptr& current_block, uint64_t current_rva);
    };

    class preopt_block
    {
    public:
        void init(dasm::basic_block* block);

        bool has_head() const;
        block_ptr get_entry();

        dasm::basic_block* get_original_block() const;
        block_ptr get_head();
        void clear_head();

        std::vector<block_ptr> get_body();
        block_ptr get_tail();

        void add_body(const block_ptr& block);

    private:
        dasm::basic_block* original_block = nullptr;

        block_ptr head;
        std::vector<block_ptr> body;
        block_ptr tail;
    };
}
