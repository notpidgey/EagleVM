#pragma once
#include "eaglevm-core/obfuscation/mba/variable/mba_var.h"

namespace eagle::mba
{
    class mba_var_exp : public mba_variable
    {
    public:
        std::array<std::unique_ptr<mba_variable>, 2> vars;
        truth_operator operation;

        // mba_var_exp(mba_var&& v1, mba_var&& v2, truth_operator v_operation, variable_modifier modifier = mod_none);
        // mba_var_exp(const mba_variable& v1, const mba_variable& v2, truth_operator v_operation, variable_modifier modifier = mod_none);
        mba_var_exp();
        mba_var_exp(const mba_var_exp& other);
        mba_var_exp(std::unique_ptr<mba_variable> v1, std::unique_ptr<mba_variable> v2, truth_operator operation, variable_modifier modifier = mod_none);

        std::string print() const override;
        void expand(const std::unique_ptr<mba_variable>& x, const std::unique_ptr<mba_variable>& y) override;
        void walk(const std::function<void(mba_var_exp*)>& walk_callback) override;
        void walk_bottom(const std::function<void(mba_var_exp*)>& walk_callback) override;

        std::unique_ptr<mba_variable> clone() override;
        std::unique_ptr<mba_var_exp> clone_exp();
    };
}
