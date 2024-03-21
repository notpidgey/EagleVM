#pragma once

enum class vm_op_action
{
    action_none,

    // 64 bit address of operand
    action_address,

    // value of operand
    action_value,

    // VREGS displacement of target register
    action_reg_address,
};