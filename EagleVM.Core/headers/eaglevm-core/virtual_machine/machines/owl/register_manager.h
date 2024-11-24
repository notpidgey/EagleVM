#pragma once
#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include "eaglevm-core/virtual_machine/machines/owl/settings.h"
#include "eaglevm-core/codec/zydis_defs.h"

#define CREATE_INDEX(y, v) inline static uint8_t index_##y = v;

namespace eagle::virt::owl
{
    using reg_range = std::pair<uint16_t, uint16_t>;

    struct reg_mapped_range
    {
        reg_range source_range;
        reg_range dest_range;

        codec::reg dest_reg = codec::reg::none;
    };

    using register_manager_ptr = std::shared_ptr<class register_manager>;


    class register_manager
    {
    public:
        static uint8_t index_vflags = 0;
        static uint8_t index_vsp = 1;

        explicit register_manager(const settings_ptr& settings_info);

        /**
        * initialize vm_order based on "settings"
        * initializes dest_register_map with GPR registers unused by the VM
        * initializes dest_register_map with all XMM registers
        */
        void init_reg_order();

        /**
        * for all 16 GPR registers r0-r15, create a scatter map of where each bit of that register will be located
        * the results of the scatter are stored in source_register_map[GPR]
        * the mappings for each destination register are located in dest_register_map[GPR]
        *
        * requires: init_reg_order must be called prior to function call
        */
        void create_mappings();
        std::pair<uint32_t, codec::reg_size> get_stack_displacement(codec::reg reg) const;

        /**
        *
        * @param reg x86 scratch register
        * @return virtual machine registers in which the scratch register is split into
        */
        std::vector<reg_mapped_range> get_register_mapped_ranges(codec::reg reg);

        /**
        *
        * @param reg x86 virtual machine register
        * @return ranges which are occupied in "reg"
        */
        std::vector<reg_range> get_occupied_ranges(codec::reg reg);

        /**
        *
        * @param reg x86 virtual machine register
        * @return ranges which are not occupied in "reg"
        */
        std::vector<reg_range> get_unoccupied_ranges(codec::reg reg);

        [[nodiscard]] std::vector<codec::reg> get_unreserved_temp_xmm() const;
        [[nodiscard]] codec::reg get_reserved_temp_xmm(uint8_t i) const;

        template <typename T>
        void enumerate_ymm(const T& enumerable, const bool from_back = false)
        {
            if (from_back)
                std::ranges::for_each(push_order, enumerable);
            else
                std::for_each(push_order.rbegin(), push_order.rend(), enumerable);
        }

        /**
        * @return GPR registers in order r0-r15
        */
        static std::array<codec::reg, 16> get_gpr64_regs();
        static std::array<codec::reg, 16> get_xmm_regs();

        std::pair<codec::reg, uint16_t> get_gpr_map(codec::reg gpr);
        std::pair<codec::reg, uint16_t> get_vsp_map();
        std::array<codec::reg, 10> get_stack_order();

        codec::reg get_vm_reg(uint8_t idx);

    private:
        settings_ptr settings;

        std::unordered_map<codec::reg, std::vector<reg_mapped_range>> source_register_map;
        std::unordered_map<codec::reg, std::vector<reg_range>> dest_register_map;

        /**
        * order 0-first 31-last in which registers have been pushed to the the stack
        */
        std::array<codec::reg, 16 + 16> push_order{ };

        std::array<codec::reg, 4> save_state_ymm;
        std::array<codec::reg, 8> stack_ymm;
    };
}
