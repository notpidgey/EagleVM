#pragma once
#include "eaglevm-core/obfuscation/mba/variable/mba_var.h"

namespace eagle::mba
{
    enum variable_xy
    {
        var_x,
        var_y
    };

    class mba_var_xy : public mba_variable
    {
    public:
        variable_xy variable_type;

        mba_var_xy(variable_xy var, variable_modifier modifier = mod_none);

        std::string print() const override;
        void expand(const std::unique_ptr<mba_variable>& x, const std::unique_ptr<mba_variable>& y) override;
        void walk(const std::function<void(mba_var_exp*)>& walk_callback) override;
        void walk_bottom(const std::function<void(mba_var_exp*)>& walk_callback) override;

        std::unique_ptr<mba_variable> clone() override;
    };
}
