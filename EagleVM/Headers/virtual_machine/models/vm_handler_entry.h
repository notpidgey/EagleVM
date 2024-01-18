#pragma once
#include <optional>
#include <array>
#include <functional>

#include "util/zydis_helper.h"

class code_label;

//may a kind soul please pr this and help me with finding a better way of doing this : )
struct vm_handler_entry
{
    code_label* handler_label;
    std::function<handle_instructions(reg_size)> creation_binder;

    //each handler might have a specific handler for qword, dwords, words
    //therefore, there may be multiple handlers stored in different locations
    std::array<reg_size, 4> supported_handler_size = {};
    std::array<uint32_t*, 4> handler_va = {};
};