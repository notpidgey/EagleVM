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

    struct complex_load_info
    {
        std::vector<std::pair<reg_range, reg_range>> complex_mapping;
        std::pair<reg_range, reg_range> get_mapping(uint16_t bit);
    };

    using inst_handlers_ptr = std::shared_ptr<class handler_manager>;
    using machine_ptr = std::shared_ptr<class machine>;

    class handler_manager
    {
    public:
        handler_manager(const machine_ptr& machine, register_manager_ptr regs,
            register_context_ptr regs_64_context, register_context_ptr regs_128_context,
            settings_ptr settings);

        asmb::code_label_ptr get_instruction_handler(codec::mnemonic mnemonic, const ir::x86_operand_sig& operand_sig);
        asmb::code_label_ptr get_instruction_handler(codec::mnemonic mnemonic, const ir::handler_sig& handler_sig);
        asmb::code_label_ptr get_instruction_handler(codec::mnemonic mnemonic, uint64_t handler_sig);
        asmb::code_label_ptr get_instruction_handler(codec::mnemonic mnemonic, int len, codec::reg_size size);

        asmb::code_label_ptr get_vm_enter();
        asmb::code_label_ptr get_vm_exit();
        asmb::code_label_ptr get_rlfags_load();
        asmb::code_label_ptr get_rflags_store();

        codec::reg get_push_working_register() const;
        codec::reg get_pop_working_register() const;

        asmb::code_label_ptr get_push(codec::reg target_reg, codec::reg_size size);
        asmb::code_label_ptr get_pop(codec::reg target_reg, codec::reg_size size);

        void call_vm_handler(const asmb::code_container_ptr& container, const asmb::code_label_ptr& label) const;

        /**
         * append to the current working block a call or inlined code to load specific register
         * it is not garuanteed these instructions will be the same per call
         * @param register_to_load
         * @param destination
         */
        std::pair<asmb::code_label_ptr, codec::reg> load_register(codec::reg register_to_load, const ir::discrete_store_ptr& destination);
        std::pair<asmb::code_label_ptr, codec::reg> load_register(codec::reg register_to_load, codec::reg load_destination);
        std::tuple<asmb::code_label_ptr, codec::reg, complex_load_info> load_register_complex(codec::reg register_to_load,
            const ir::discrete_store_ptr& destination);

        /**
         * append to the current working block a call or inlined code to store a value in a specific register
         * it is not garuanteed these instructions will be the same per call
         * @param register_to_store_into
         * @param source
         */
        std::pair<asmb::code_label_ptr, codec::reg> store_register(codec::reg register_to_store_into, const ir::discrete_store_ptr& source);
        std::pair<asmb::code_label_ptr, codec::reg> store_register(codec::reg register_to_store_into, codec::reg source);
        std::tuple<asmb::code_label_ptr, codec::reg> store_register_complex(codec::reg register_to_store_into, codec::reg source,
            const complex_load_info& load_info);

        static complex_load_info generate_complex_load_info(const uint16_t start_bit, const uint16_t end_bit);
        static std::vector<reg_mapped_range> apply_complex_mapping(const complex_load_info& load_info, const std::vector<reg_mapped_range>& register_ranges);

        asmb::code_label_ptr resolve_complexity(const ir::discrete_store_ptr& source, const complex_load_info& load_info);

        std::vector<asmb::code_container_ptr> build_handlers();

        asmb::code_container_ptr build_vm_enter();
        asmb::code_container_ptr build_vm_exit();

        asmb::code_container_ptr build_rflags_load();
        asmb::code_container_ptr build_rflags_store();

        std::vector<asmb::code_container_ptr> build_push();
        std::vector<asmb::code_container_ptr> build_pop();

        std::vector<asmb::code_container_ptr> build_jcc();

    private:
        std::weak_ptr<machine> machine_inst;
        settings_ptr settings;

        register_manager_ptr regs;
        register_context_ptr regs_64_context;
        register_context_ptr regs_128_context;

        tagged_handler vm_enter;
        tagged_handler vm_exit;

        tagged_handler vm_rflags_load;
        tagged_handler vm_rflags_store;

        std::unordered_map<codec::reg, tagged_handler_data_pair> vm_push;
        std::unordered_map<codec::reg, tagged_handler_data_pair> vm_pop;

        using jcc_mask_expected = std::pair<uint32_t, uint32_t>;
        std::unordered_map<ir::exit_condition, tagged_handler_data_pair> vm_jcc;

        std::vector<tagged_handler_data_pair> register_load_handlers;
        std::vector<tagged_handler_data_pair> register_store_handlers;

        std::vector<tagged_handler_data_pair> complex_resolve_handlers;

        uint16_t vm_overhead;
        uint16_t vm_stack_regs;
        uint16_t vm_call_stack;

        using tagged_handler_id = std::pair<codec::mnemonic, uint64_t>;
        using tagged_handler_label = std::pair<tagged_handler_id, asmb::code_label_ptr>;
        std::vector<tagged_handler_label> tagged_instruction_handlers;

        void load_register_internal(codec::reg load_destination, const asmb::code_container_ptr& out,
            const std::vector<reg_mapped_range>& ranges_required) const;
        void store_register_internal(codec::reg source_register, const asmb::code_container_ptr& out,
            const std::vector<reg_mapped_range>& ranges_required) const;

        static void trim_ranges(std::vector<reg_mapped_range>& ranges_required, codec::reg target);

        [[nodiscard]] std::vector<reg_mapped_range> get_relevant_ranges(codec::reg source_reg) const;
        void create_vm_return(const asmb::code_container_ptr& container) const;
        static codec::reg_size load_store_index_size(uint8_t index);

        std::vector<asmb::code_container_ptr> build_instruction_handlers();
    };
}
