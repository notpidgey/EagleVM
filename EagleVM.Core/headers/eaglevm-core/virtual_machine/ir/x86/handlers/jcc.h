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

    private:
        ir_insts write_condition_jump(uint64_t flag_mask) const;
        ir_insts write_bitwise_condition(const base_command_ptr& bitwise, uint64_t flag_mask_one, uint64_t flag_mask_two) const;
        ir_insts write_check_register(codec::reg reg) const;
        ir_insts write_jle() const;

        ir_insts write_flag_operation(const std::function<std::vector<base_command_ptr>()>& operation_generator) const;

        static ir_insts load_isolated_flag(uint64_t flag_mask);
        static uint64_t get_flag_for_condition(exit_condition condition);
        static codec::reg get_register_for_condition(exit_condition condition);
    };
}

namespace eagle::ir::lifter
{
    class jcc : public base_x86_translator
    {
    public:
        using base_x86_translator::base_x86_translator;
        bool translate_to_il(uint64_t original_rva, x86_cpu_flag flags = NONE) override;

    private:
        static std::pair<exit_condition, bool> get_exit_condition(codec::mnemonic mnemonic);
    };
}
