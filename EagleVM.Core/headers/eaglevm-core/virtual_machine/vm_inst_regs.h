#pragma once
#include <array>
#include <algorithm>
#include <vector>
#include <random>

#include "eaglevm-core/virtual_machine/models/vm_defs.h"
#include "eaglevm-core/codec/zydis_helper.h"

namespace eagle::virt
{
    class vm_inst_regs
    {
    public:
        vm_inst_regs();

        void init_reg_order();
        codec::zydis_register get_reg(uint8_t target) const;
        std::pair<uint32_t, codec::reg_size> get_stack_displacement(codec::zydis_register reg) const;

        void enumerate(const std::function<void(codec::zydis_register)>& enumerable, bool from_back = false);

    private:
        std::array<codec::zydis_register, 16> reg_stack_order_;
        std::array<codec::zydis_register, 16> reg_vm_order_;
    };
}
