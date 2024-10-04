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

        void write_basic_compare()
        void write_basic_jump(uint64_t jcc_mask, std::array<il_exit_result, 2> exits) const;
        void write_bitwise_compare(codec::mnemonic mnemonic, uint64_t flag_mask_one, uint64_t flag_mask_two, std::array<il_exit_result, 2> exits) const;
        void write_check_register(codec::reg reg, std::array<il_exit_result, 2> exits) const;
        void write_jle(std::array<il_exit_result, 2> exits) const;
    };
}