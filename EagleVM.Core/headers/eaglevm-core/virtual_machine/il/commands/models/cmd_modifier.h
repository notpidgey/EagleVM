#pragma once
#include <variant>
#include "eaglevm-core/virtual_machine/il/models/il_reg.h"

namespace eagle::il
{
    enum class modifier_type
    {
        inv,
        imm,
        reg
    };

    using modifier_imm = uint64_t;
    using modifier_value = std::variant<reg_v, modifier_imm>;

    struct modifier
    {
        modifier(reg_v reg,)
        {

        }

        modifier_type type = modifier_type::imm;
        modifier_value value = static_cast<modifier_imm>(0);
    };
}
