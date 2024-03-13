#pragma once
#include <array>
#include <algorithm>
#include <vector>
#include <random>

#include "models/vm_defs.h"
#include "util/zydis_defs.h"
#include "util/zydis_helper.h"

class vm_inst_regs
{
public:
    vm_inst_regs();

    void init_reg_order();
    zydis_register get_reg(uint8_t target) const;
    std::pair<uint32_t, reg_size> get_stack_displacement(zydis_register reg) const;

    void enumerate(const std::function<void(zydis_register)>& enumerable, bool from_back = false);

private:
    std::array<zydis_register, 16> reg_stack_order_;
    std::array<zydis_register, 16> reg_vm_order_;
};