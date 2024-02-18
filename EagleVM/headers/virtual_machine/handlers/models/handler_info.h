#pragma once
#include "util/zydis_helper.h"

class code_label;
struct handler_info
{
    reg_size instruction_width = bit64;
    uint8_t operand_count = 2;

    std::function<void(function_container& container, reg_size size)> construct = nullptr;
    code_label* target_label = nullptr;
};