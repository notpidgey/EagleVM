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
         * @param reg
         * @param destination
         * @param use_handler
         */
        void load_register(codec::reg reg, const ir::discrete_store_ptr& destination);

        /**
         * append to the current working block a call or inlined code to store a value in a specific register
         * it is not garuanteed these instructions will be the same per call
         * @param reg
         * @param source
         * @param use_handler
         */
        void store_register(codec::reg reg, const ir::discrete_store_ptr& source);

        void call_handler(asmb::code_label_ptr label);

    private:
        asmb::code_container_ptr working_block;
        std::deque<asmb::code_container_ptr> complete_containers;

        machine_ptr machine;
        inst_regs_ptr regs;
        settings_ptr settings;

        std::unordered_map<codec::reg, tagged_handler> register_load_handlers;
        std::unordered_map<codec::reg, tagged_handler> register_store_handlers;
    };
}
