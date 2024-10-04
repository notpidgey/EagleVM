#pragma once
#include "eaglevm-core/virtual_machine/ir/x86/base_handler_gen.h"
#include "eaglevm-core/virtual_machine/ir/x86/base_x86_translator.h"

namespace eagle::ir::handler
{
    class jcc : public base_handler_gen
    {
    public:
        jcc();
        ir_insts gen_handler(uint64_t target_handler_id) override;
    };
}

namespace eagle::ir::lifter
{
    class jcc : public base_x86_translator
    {
    public:
        using base_x86_translator::base_x86_translator;
        virtual bool translate_to_il(uint64_t original_rva, x86_cpu_flag flags = NONE);

    private:
        std::pair<exit_condition, bool> get_exit_condition(const codec::mnemonic mnemonic);

        void jcc::write_condition_jump(uint64_t flag_mask, const std::array<il_exit_result, 2>& exits) const;
        void jcc::write_bitwise_condition(codec::mnemonic mnemonic, uint64_t flag_mask_one, uint64_t flag_mask_two,
            const std::array<il_exit_result, 2>& exits) const;
        void jcc::write_check_register(codec::reg reg, const std::array<il_exit_result, 2>& exits) const;
        void jcc::write_jle(const std::array<il_exit_result, 2>& exits) const;
        void write_flag_operation(uint64_t flag_mask, std::array<il_exit_result, 2> exits,
            const std::function<std::vector<base_command_ptr>(uint64_t)>& operation_generator) const;

        static auto get_flag_for_condition(exit_condition condition) -> uint64_t;
        static codec::reg get_register_for_condition(exit_condition condition);
    };
}
