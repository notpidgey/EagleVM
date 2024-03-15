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

#include "eaglevm-core/virtual_machine/vm_inst_regs.h"

#include "eaglevm-core/util/zydis_helper.h"
#include "eaglevm-core/util/util.h"

class vm_inst_regs;
class inst_handler_entry;
class vm_handler_entry;

class vm_inst_handlers
{
public:
    std::unordered_map<int, inst_handler_entry*> inst_handlers;
    std::unordered_map<int, vm_handler_entry*> v_handlers;

    explicit vm_inst_handlers(vm_inst_regs* push_order);
    void setup_vm_mapping();

private:
    vm_inst_regs* rm_;
};