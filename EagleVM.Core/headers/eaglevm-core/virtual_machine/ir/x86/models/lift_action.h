#pragma once

namespace eagle::ir::lifter
{
    enum op_action
    {
        action_none = 0b0,

        // 64 bit address of operand
        action_address = 0b1,

        // value of operand
        action_value = 0b10,

        // VREGS displacement of target register
        action_reg_offset = 0b100,
    };
}
