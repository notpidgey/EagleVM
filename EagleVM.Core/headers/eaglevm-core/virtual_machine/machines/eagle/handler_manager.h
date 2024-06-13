#pragma once
#include <deque>

#include "eaglevm-core/codec/zydis_enum.h"
#include "eaglevm-core/compiler/code_container.h"

#include "eaglevm-core/virtual_machine/ir/models/ir_discrete_reg.h"

#include "eaglevm-core/virtual_machine/machines/eagle/settings.h"
#include "eaglevm-core/virtual_machine/machines/eagle/register_manager.h"

#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_handler_signature.h"
#include "eaglevm-core/virtual_machine/ir/commands/models/cmd_operand_signature.h"

namespace eagle::virt
{
    using register_context_ptr = std::shared_ptr<class register_context>;
}

namespace eagle::virt::eg
{
    using tagged_handler_data_pair = std::pair<asmb::code_container_ptr, asmb::code_label_ptr>;

    struct tagged_variant_handler
    {
        std::vector<tagged_handler_data_pair> variant_pairs{ };

        tagged_handler_data_pair add_pair();
        tagged_handler_data_pair get_first_pair();
    };

    class tagged_handler
    {
    public:
        tagged_handler()
        {
            data = { asmb::code_container::create(), asmb::code_label::create() };
            tagged = false;
        }

        tagged_handler_data_pair get_pair();
        asmb::code_container_ptr get_container();
        asmb::code_label_ptr get_label();

        void tag() { tagged = true; }
        bool get_tagged() const { return tagged; }

    private:
        tagged_handler_data_pair data;
        bool tagged = false;
    };

    using inst_handlers_ptr = std::shared_ptr<class handler_manager>;
    using machine_ptr = std::shared_ptr<class machine>;

    class handler_manager
    {
    public:
        handler_manager(machine_ptr machine, register_manager_ptr regs, const register_context_ptr& regs_context, settings_ptr settings);

        /**
         * sets the block for the current inst_handlers context to be appeneded to
         * @param block target block pointer
         */
        void set_working_block(const asmb::code_container_ptr& block);

        asmb::code_label_ptr get_instruction_handler(codec::mnemonic mnemonic, const ir::x86_operand_sig& operand_sig);
        asmb::code_label_ptr get_instruction_handler(codec::mnemonic mnemonic, const ir::handler_sig& handler_sig);
        asmb::code_label_ptr get_instruction_handler(codec::mnemonic mnemonic, std::string handler_sig);
        asmb::code_label_ptr get_instruction_handler(codec::mnemonic mnemonic, int len, codec::reg_size size);

        void call_vm_handler(asmb::code_label_ptr label);

        asmb::code_label_ptr get_vm_enter();
        asmb::code_container_ptr build_vm_enter();

        asmb::code_label_ptr get_vm_exit();
        asmb::code_container_ptr build_vm_exit();

        asmb::code_label_ptr get_rlfags_load();
        asmb::code_container_ptr build_rflags_load();

        asmb::code_label_ptr get_rflags_store();
        asmb::code_container_ptr build_rflags_store();

        codec::reg get_push_working_register();
        std::vector<asmb::code_container_ptr> build_push();

        codec::reg get_pop_working_register();
        std::vector<asmb::code_container_ptr> build_pop();

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

        asmb::code_label_ptr get_push(eagle::codec::reg target_reg, eagle::codec::reg_size size);
        asmb::code_label_ptr get_pop(eagle::codec::reg reg, eagle::codec::reg_size size);

        std::vector<asmb::code_container_ptr> build_handlers();

    private:
        asmb::code_container_ptr working_block;

        machine_ptr machine;
        settings_ptr settings;

        register_manager_ptr regs;
        register_context_ptr regs_context;

        tagged_handler vm_enter;
        tagged_handler vm_exit;

        tagged_handler vm_rflags_load;
        tagged_handler vm_rflags_store;

        std::unordered_map<codec::reg, tagged_handler_data_pair> vm_push;
        std::unordered_map<codec::reg, tagged_handler_data_pair> vm_pop;

        std::unordered_map<codec::reg, tagged_variant_handler> register_load_handlers;
        std::unordered_map<codec::reg, tagged_variant_handler> register_store_handlers;

        uint16_t vm_overhead;
        uint16_t vm_stack_regs;
        uint16_t vm_call_stack;

        std::vector<asmb::code_container_ptr> build_instruction_handlers();

        using tagged_handler_id = std::pair<codec::mnemonic, std::string>;
        using tagged_handler_label = std::pair<tagged_handler_id, asmb::code_label_ptr>;
        std::vector<tagged_handler_label> tagged_instruction_handlers;

        [[nodiscard]] std::vector<reg_mapped_range> get_relevant_ranges(codec::reg source_reg) const;
        std::pair<bool, codec::reg> handle_reg_handler_query(std::unordered_map<codec::reg, tagged_variant_handler>& handler_storage, codec::reg reg);

        void create_vm_return(const asmb::code_container_ptr& container) const;

        static codec::reg_size load_store_index_size(const uint8_t index);
    };
}
