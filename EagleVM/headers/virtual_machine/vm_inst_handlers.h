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

#include "virtual_machine/handlers/handler/inst_handler_entry.h"
#include "virtual_machine/handlers/handler/vm_handler_entry.h"
#include "virtual_machine/vm_inst_handlers.h"
#include "virtual_machine/vm_inst_regs.h"

#include "util/zydis_helper.h"
#include "util/util.h"

class vm_inst_handlers
{
public:
    std::unordered_map<int, inst_handler_entry*> inst_handlers;
    std::unordered_map<int, vm_handler_entry*> v_handlers;

    explicit vm_inst_handlers(vm_inst_regs* push_order);
    vm_inst_handlers();

    void setup_vm_mapping();

private:
    vm_inst_regs* rm_;
};