#pragma once
#include "variable/mba_var.h"
#include "variable/mba_const.h"
#include "variable/mba_exp.h"
#include "variable/mba_xy.h"

class mba_gen
{
public:
    mba_gen();
    std::string create_tree(truth_operator none, uint32_t max_expansions, uint8_t equal_expansions);

private:
    // mba_equality_map<mba_var_exp, mba_var_exp> mba_communitive_truth;
    std::unordered_map<truth_operator, mba_var_exp> mba_base_truth;
    std::vector<std::pair<mba_var_exp, mba_var_exp>> mba_simple_truth;
    std::vector<mba_var_exp> mba_variable_truth;
    std::vector<mba_var_exp> mba_zero_truth;

    void bottom_expand_base(std::unique_ptr<mba_var_exp>& exp);
    void bottom_expand_simple(std::unique_ptr<mba_var_exp>& exp);
    void bottom_expand_variable(std::unique_ptr<mba_var_exp>& exp);
};