#pragma once
#include <optional>
#include <array>
#include <functional>

#include "util/zydis_helper.h"
#include "util/section/function_container.h"

class code_label;
class vm_handler_entry
{
public:
    vm_handler_entry();
    code_label* get_handler_va(reg_size size);

    function_container& construct_handler();

private:
    bool is_vm_handler;
    function_container container;

    std::vector<reg_size> supported_sizes;
    std::unordered_map<reg_size, code_label*> supported_handlers;

    virtual handle_instructions construct_single(reg_size size) = 0;
};