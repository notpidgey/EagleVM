#pragma once
#include <array>
#include <memory>
#include <vector>

#include "eaglevm-core/virtual_machine/machines/eagle/settings.h"
#include "eaglevm-core/codec/zydis_defs.h"

#define CREATE_INDEX(y, v) inline static uint8_t index_##y = v;

namespace eagle::virt::eg
{
    using reg_range = std::pair<uint16_t, uint16_t>;

    struct reg_mapped_range
    {
        reg_range source_range;
        reg_range dest_range;

        codec::reg dest_reg = codec::reg::none;
    };

    using inst_regs_ptr = std::shared_ptr<class inst_regs>;

    class inst_regs
    {
    public:
        explicit inst_regs(const settings_ptr& settings_info);

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

        std::vector<codec::reg> get_unreserved_temp() const;
        codec::reg get_reserved_temp(uint8_t i) const;

        std::vector<codec::reg> get_unreserved_temp_xmm() const;
        codec::reg get_reserved_temp_xmm(uint8_t i) const;

        /**
         * value containing the number of x86 registers being used by the virtual machine
         */
        static uint8_t num_v_regs;
        /**
         *  constant value always equals to 16
         */
        static uint8_t num_gpr_regs;

        /**
         * index of VIP virtual machine register in vm_order
         */
        static uint8_t index_vip;
        /**
         * index of VSP virtual machine register in vm_order
         */
        static uint8_t index_vsp;
        /**
         * index of VREGS virtual machine register in vm_order
         */
        static uint8_t index_vregs;
        /**
         * index of VCS virtual machine register in vm_order
         */
        static uint8_t index_vcs;

        /**
         * index of VCSRET virtual machine register in vm_order
         */
        static uint8_t index_vcsret;

        /**
         * index of VBASE virtual machine register in vm_order
         */
        static uint8_t index_vbase;

    private:
        settings_ptr settings;

        std::unordered_map<codec::reg, std::vector<reg_mapped_range>> source_register_map;
        std::unordered_map<codec::reg, std::vector<reg_range>> dest_register_map;

        /**
         * order 0-first 31-last in which registers have been pushed to the the stack
         */
        std::array<codec::reg, 32> push_order{ };

        /**
         * when assigning virtual machine registers such as VREGS, VIP, VSP, VTEMP(x)
         * they are accessed via a predefined index of this array
         */
        std::array<codec::reg, 16> virtual_order_gpr{ };

        /**
         * when assigning virtual
         */
        std::array<codec::reg, 16> virtual_order_xmm{ };

        uint8_t num_v_temp_unreserved;
        uint8_t num_v_temp_reserved;

        uint8_t num_v_temp_xmm_unreserved;
        uint8_t num_v_temp_xmm_reserved;

        /**
         * @return GPR registers in order r0-r15
         */
        static std::array<codec::reg, 16> get_gpr64_regs();
        static std::array<codec::reg, 16> get_xmm_regs();
    };
}
