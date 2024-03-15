#pragma once
#include "eaglevm-core/obfuscation/mba/variable/mba_exp.h"

namespace mba_math
{
    std::shared_ptr<mba_var_exp> create_invertible_poly();
    std::shared_ptr<mba_var_exp> create_invertible_linear();
}