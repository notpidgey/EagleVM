#pragma once
#include <array>
#include <algorithm>
#include <functional>
#include <random>

#include "eaglevm-core/codec/zydis_enum.h"

namespace eagle::virt::pidg
{
    class inst_regs;
    using vm_inst_regs_ptr = std::shared_ptr<inst_regs>;

    class inst_regs
    {
    public:
        inst_regs();

        void init_reg_order();
        codec::zydis_register get_reg(uint8_t target) const;
        std::pair<uint32_t, codec::reg_size> get_stack_displacement(codec::zydis_register reg) const;

        void enumerate(const std::function<void(codec::zydis_register)>& enumerable, bool from_back = false);

    private:
        std::array<codec::zydis_register, 16> stack_order;
        std::array<codec::zydis_register, 16> vm_order;
    };
}

#define NUM_OF_VREGS 8
#define I_VIP 0
#define I_VSP 1
#define I_VREGS 2

#define I_VTEMP 3
#define I_VTEMP2 4

#define I_VCALLSTACK 5
#define I_VCSRET 6
#define I_VBASE 7

#define MNEMONIC_VM_ENTER 0
#define MNEMONIC_VM_EXIT 1
#define MNEMONIC_VM_LOAD_REG 2
#define MNEMONIC_VM_STORE_REG 3

#define MNEMONIC_VM_RFLAGS_ACCEPT 4
#define MNEMONIC_VM_RFLAGS_LOAD 5

#define VIP         rm_->get_reg(I_VIP)
#define VSP         rm_->get_reg(I_VSP)
#define VREGS       rm_->get_reg(I_VREGS)
#define VTEMP       rm_->get_reg(I_VTEMP)
#define VTEMP2      rm_->get_reg(I_VTEMP2)
#define VCS         rm_->get_reg(I_VCALLSTACK)
#define VCSRET      rm_->get_reg(I_VCSRET)
#define VBASE       rm_->get_reg(I_VBASE)
