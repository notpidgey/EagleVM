#pragma once
#include <set>
#include <unordered_map>
#include "commands/cmd_branch.h"

#include "eaglevm-core/disassembler/basic_block.h"
#include "eaglevm-core/disassembler/analysis/liveness.h"
#include "eaglevm-core/virtual_machine/ir/block.h"
#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"
#include "models/ir_branch_info.h"

namespace eagle::dasm
{
    using segment_dasm_ptr = std::shared_ptr<class segment_dasm>;
}

namespace eagle::ir
{
    class preopt_block;
    using preopt_block_ptr = std::shared_ptr<preopt_block>;
    using preopt_block_vec = std::vector<preopt_block_ptr>;

    using preopt_vm_id = std::pair<preopt_block_ptr, uint32_t>;
    using flat_block_vmid = std::pair<std::vector<block_ptr>, uint32_t>;

    class ir_translator : public std::enable_shared_from_this<ir_translator>
    {
    public:
        explicit ir_translator(const dasm::segment_dasm_ptr& seg_dasm, dasm::analysis::liveness* liveness = nullptr);

        std::vector<preopt_block_ptr> translate();
        std::vector<flat_block_vmid> flatten(
            std::unordered_map<preopt_block_ptr, uint32_t>& block_vm_ids,
            std::unordered_map<preopt_block_ptr, block_ptr>& block_tracker
        );
        std::vector<flat_block_vmid> optimize(
            std::unordered_map<preopt_block_ptr, uint32_t>& block_vm_ids,
            std::unordered_map<preopt_block_ptr, block_ptr>& block_tracker,
            const std::vector<preopt_block_ptr>& extern_call_blocks
        );

        dasm::basic_block_ptr map_basic_block(const preopt_block_ptr& preopt_target);
        preopt_block_ptr map_preopt_block(const dasm::basic_block_ptr& basic_block);

        /**
         *
         * @param inst_rva can be any rva that is within the target block's bounds (including the rva of the branching instruction)
         * @return branching information of the found block within the translator's context
         */
        branch_info get_branch_info(uint32_t inst_rva);

    private:
        dasm::segment_dasm_ptr dasm;
        dasm::analysis::liveness* dasm_liveness;
        std::unordered_map<dasm::basic_block_ptr, preopt_block_ptr> bb_map;

        void optimize_heads(
            std::unordered_map<preopt_block_ptr, uint32_t>& block_vm_ids,
            std::unordered_map<preopt_block_ptr, block_ptr>& block_tracker,
            const std::vector<preopt_block_ptr>& extern_call_blocks
        );

        void optimize_body_to_tail(
            std::unordered_map<preopt_block_ptr, uint32_t>& block_vm_ids,
            std::unordered_map<preopt_block_ptr, block_ptr>& block_tracker,
            const std::vector<preopt_block_ptr>& extern_call_blocks
        );

        void optimize_same_vm(
            std::unordered_map<preopt_block_ptr, uint32_t>& block_vm_ids,
            std::unordered_map<preopt_block_ptr, block_ptr>& block_tracker,
            const std::vector<preopt_block_ptr>& extern_call_blocks
        );

        preopt_block_ptr translate_block_split(dasm::basic_block_ptr bb);
        static std::pair<exit_condition, bool> get_exit_condition(codec::mnemonic mnemonic);
        static void handle_block_command(codec::dec::inst_info decoded_inst, const block_ptr& current_block, uint64_t current_rva);
    };

    struct preopt_block
    {
        void init(const dasm::basic_block_ptr& block);

        dasm::basic_block_ptr original_block = nullptr;

        block_virt_ir_ptr head;
        std::vector<block_ptr> body;
        block_x86_ir_ptr tail;
    };
}
