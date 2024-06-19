#pragma once
#include <array>
#include <algorithm>
#include <functional>
#include <random>

#include "eaglevm-core/virtual_machine/machines/pidgeon/settings.h"
#include "eaglevm-core/codec/zydis_enum.h"

namespace eagle::virt::pidg
{
    class inst_regs;
    using vm_inst_regs_ptr = std::shared_ptr<inst_regs>;

    class inst_regs
    {
    public:
        explicit inst_regs(uint8_t temp_count, settings_ptr settings);

        void init_reg_order();
        codec::reg get_reg(uint8_t target) const;
        codec::reg get_reg_temp(uint8_t target) const;
        std::vector<codec::reg> get_availiable_temp() const;

        std::pair<uint32_t, codec::reg_size> get_stack_displacement(codec::reg reg) const;

        void enumerate(const std::function<void(codec::reg)>& enumerable, const bool from_back = false);

    private:
        settings_ptr settings;

        std::array<codec::reg, 16> stack_order;
        std::array<codec::reg, 16> vm_order;

        uint8_t temp_variables;
        uint8_t number_of_vregs;
    };
}

#define NUM_OF_VREGS 6
#define I_VIP 0
#define I_VSP 1
#define I_VREGS 2

#define I_VCALLSTACK 3
#define I_VCSRET 4
#define I_VBASE 5
