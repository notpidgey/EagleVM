#pragma once
#include <variant>
#include "eaglevm-core/virtual_machine/ir/models/il_reg.h"

namespace eagle::il
{
    enum class modifier_type
    {
        inv,
        imm,
        reg
    };

    enum class modifier_op
    {
        invalid,
        add,
        sub,
        mul
    };

    using modifier_imm = uint64_t;
    using modifier_value = std::variant<reg_v, modifier_imm>;

    class modifier
    {
    public:
        explicit modifier(reg_v reg, const modifier_op m_op)
        {
            op = m_op;
            value = reg;
            type = modifier_type::reg;
        }

        explicit modifier(modifier_imm imm, const modifier_op m_op)
        {
            op = m_op;
            value = imm;
            type = modifier_type::imm;
        }

        modifier_op op = modifier_op::invalid;
        modifier_type type = modifier_type::imm;
        modifier_value value = static_cast<modifier_imm>(0);
    };
}
