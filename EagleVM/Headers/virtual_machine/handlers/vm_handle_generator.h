#pragma once

#include <Zydis/Zydis.h>
#include <Zycore/LibC.h>

#include <vector>
#include <functional>
#include <iostream>
#include <cstddef>
#include <array>
#include <map>
#include <ranges>

#include "virtual_machine/handlers/vm_handler_entry.h"
#include "virtual_machine/vm_register_manager.h"
#include "util/zydis_helper.h"
#include "util/util.h"
#include "util/math.h"

class vm_handle_generator
{
public:
    std::map<int, vm_handler_entry> vm_handlers;

    vm_handle_generator();
    explicit vm_handle_generator(vm_register_manager* push_order);

    void setup_vm_mapping();
    void setup_enc_constants();

    handle_instructions create_vm_enter_jump(uint32_t va_vm_enter, uint32_t va_protected);

private:
    vm_register_manager* rm_;
};