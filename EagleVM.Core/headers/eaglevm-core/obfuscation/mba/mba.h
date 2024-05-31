#pragma once
#include "eaglevm-core/obfuscation/mba/variable/mba_var.h"
#include "eaglevm-core/obfuscation/mba/variable/mba_const.h"
#include "eaglevm-core/obfuscation/mba/variable/mba_exp.h"
#include "eaglevm-core/obfuscation/mba/variable/mba_xy.h"

namespace eagle::mba
{
    template <typename T>
    class mba_gen
    {
    public:
        mba_gen(uint8_t gen_size);
        std::string create_tree(truth_operator none, uint32_t max_expansions, uint8_t equal_expansions);

    private:
        uint8_t gen_size;

        // mba_equality_map<mba_var_exp, mba_var_exp> mba_communitive_truth;
        std::unordered_map<truth_operator, mba_var_exp> mba_base_truth;
        std::vector<std::pair<mba_var_exp, mba_var_exp>> mba_simple_truth;
        std::vector<mba_var_exp> mba_variable_truth;
        std::vector<mba_var_exp> mba_zero_truth;

        void bottom_expand_base(std::unique_ptr<mba_var_exp>& exp);
        void bottom_expand_simple(std::unique_ptr<mba_var_exp>& exp);
        void bottom_expand_variable(std::unique_ptr<mba_var_exp>& exp);
        void bottom_insert_identity(std::unique_ptr<mba_var_exp>& exp);
        void expand_constants(std::unique_ptr<mba_var_exp>& exp);
    };
}
