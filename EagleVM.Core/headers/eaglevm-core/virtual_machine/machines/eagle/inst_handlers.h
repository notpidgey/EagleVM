#pragma once
#include <deque>

#include "eaglevm-core/codec/zydis_enum.h"
#include "eaglevm-core/compiler/code_container.h"

#include "eaglevm-core/virtual_machine/ir/models/ir_discrete_reg.h"

#include "eaglevm-core/virtual_machine/machines/eagle/settings.h"
#include "eaglevm-core/virtual_machine/machines/eagle/inst_regs.h"
#include "eaglevm-core/virtual_machine/machines/eagle/machine.h"

namespace eagle::virt::eg
{
    using tagged_handler_pair = std::pair<asmb::code_container_ptr, asmb::code_label_ptr>;

    struct tagged_handler
    {
        std::vector<tagged_handler_pair> variant_pairs{ };

        tagged_handler_pair add_pair();
        tagged_handler_pair get_first_pair();
    };

    struct load_register_lock { ir::discrete_store_ptr destination{}; ir::discrete_store_ptr temp{}; };
    struct store_register_lock { ir::discrete_store_ptr source{}; ir::discrete_store_ptr temp{}; ir::discrete_store_ptr temp2{}; };

    using inst_handlers_ptr = std::shared_ptr<class inst_handlers>;

    class inst_handlers
    {
    public:
        inst_handlers(machine_ptr machine, inst_regs_ptr regs, settings_ptr settings);

        /**
         * sets the block for the current inst_handlers context to be appeneded to
         * @param block target block pointer
         */
        void set_working_block(const asmb::code_container_ptr& block);

        /**
         * append to the current working block a call or inlined code to load specific register
         * it is not garuanteed these instructions will be the same per call
         * @param register_load
         * @param destination
         */
        void load_register(codec::reg register_load, const ir::discrete_store_ptr& destination);

        /**
         * append to the current working block a call or inlined code to store a value in a specific register
         * it is not garuanteed these instructions will be the same per call
         * @param reg
         * @param source
         */
        void store_register(codec::reg reg, const ir::discrete_store_ptr& source);

        asmb::code_label_ptr get_instruction_handler(codec::mnemonic mnemonic, const ir::x86_operand_sig& operand_sig);
        asmb::code_label_ptr get_instruction_handler(codec::mnemonic mnemonic, const ir::handler_sig& handler_sig);
        asmb::code_label_ptr get_instruction_handler(codec::mnemonic mnemonic, std::string handler_sig);
        asmb::code_label_ptr get_instruction_handler(codec::mnemonic mnemonic, int len, codec::reg_size size);

        void call_vm_handler(asmb::code_label_ptr label);

    private:
        asmb::code_container_ptr working_block;

        machine_ptr machine;
        inst_regs_ptr regs;
        settings_ptr settings;

        std::unordered_map<codec::reg, tagged_handler> register_load_handlers;
        std::unordered_map<codec::reg, tagged_handler> register_store_handlers;

        using tagged_handler_id = std::pair<codec::mnemonic, std::string>;
        using tagged_handler_label = std::pair<tagged_handler_id, asmb::code_label_ptr>;
        std::vector<tagged_handler_label> tagged_instruction_handlers;

        [[nodiscard]] std::vector<reg_mapped_range> get_relevant_ranges(codec::reg source_reg) const;
        std::pair<bool, codec::reg> handle_reg_handler_query(std::unordered_map<codec::reg, tagged_handler>& handler_storage, codec::reg reg);

        void handle_load_register_gpr64();
        void handle_load_register_xmm();
    };
}
