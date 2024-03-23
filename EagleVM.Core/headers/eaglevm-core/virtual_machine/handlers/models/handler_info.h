#pragma once
#include "eaglevm-core/virtual_machine/handlers/models/handler_override.h"
#include "eaglevm-core/util/zydis_helper.h"

class code_label;
struct handler_info
{
    reg_size instruction_width = bit64;
    uint8_t operand_count = 2;
    handler_override override = ho_default;

    code_label* target_label = nullptr;
};