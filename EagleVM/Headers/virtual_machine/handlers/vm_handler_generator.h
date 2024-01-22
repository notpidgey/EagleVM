#pragma once

#include <Zydis/Zydis.h>
#include <Zycore/LibC.h>

#include <vector>
#include <functional>
#include <iostream>
#include <cstddef>
#include <array>
#include <unordered_map>
#include <ranges>

#include "virtual_machine/handlers/vm_handler_entry.h"
#include "virtual_machine/vm_register_manager.h"
#include "util/zydis_helper.h"
#include "util/util.h"

class vm_handler_generator
{
public:
    std::unordered_map<int, vm_handler_entry*> vm_handlers;

    vm_handler_generator();
    explicit vm_handler_generator(vm_register_manager* push_order);

    void setup_vm_mapping();
    void setup_enc_constants();

    instructions_vec create_vm_enter_jump(uint32_t va_vm_enter, uint32_t va_protected);

private:
    vm_register_manager* rm_;
};