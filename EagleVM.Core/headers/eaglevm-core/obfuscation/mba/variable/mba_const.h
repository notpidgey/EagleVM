#pragma once
#include "eaglevm-core/obfuscation/mba/variable/mba_var.h"

namespace eagle::mba
{
    template <typename T>
    class mba_var_const : public mba_variable
    {
    public:
        T value;

        mba_var_const(T value, variable_modifier modifier = mod_none)
            : mba_variable(constant, modifier), value(value)
        {
        }

        std::string print() const override
        {
            return std::to_string(value);
        }

        void expand(const std::unique_ptr<mba_variable>& x, const std::unique_ptr<mba_variable>& y) override
        {
            return;
        }

        std::unique_ptr<mba_variable> clone() override
        {
            return std::make_unique<mba_var_const>(*this);
        }

        void walk(const std::function<void(mba_var_exp*)>& walk_callback) override
        {
            return;
        }

        void walk_bottom(const std::function<void(mba_var_exp*)>& walk_callback) override
        {
            return;
        }
    };
}
