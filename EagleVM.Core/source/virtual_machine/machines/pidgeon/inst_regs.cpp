#include "eaglevm-core/virtual_machine/machines/pidgeon/inst_regs.h"

#include "eaglevm-core/util/random.h"
#include "eaglevm-core/codec/zydis_helper.h"

#include "eaglevm-core/virtual_machine/models/vm_defs.h"

namespace eagle::virt::pidg
{
    inst_regs::inst_regs()
    {
        stack_order = {};
        vm_order = {};
    }

    void inst_regs::init_reg_order()
    {
        // todo: find a safer way to do this instead of enumearting enum
        for(int i = ZYDIS_REGISTER_RAX; i <= ZYDIS_REGISTER_R15; i++)
            stack_order[i - ZYDIS_REGISTER_RAX] = static_cast<codec::zydis_register>(i);

        std::ranges::shuffle(stack_order, util::ran_device::get().gen);

        bool success_shuffle = false;
        std::array<codec::zydis_register, 16> access_order = stack_order;

        while(!success_shuffle)
        {
            std::ranges::shuffle(access_order, util::ran_device::get().gen);

            success_shuffle = true;
            for(int i = 0; i < NUM_OF_VREGS; i++)
            {
                const codec::zydis_register target = access_order[i];

                // these are registers we do not want to be using as VM registers
                if(target == ZYDIS_REGISTER_RIP || target == ZYDIS_REGISTER_RAX || target == ZYDIS_REGISTER_RSP)
                {
                    success_shuffle = false;
                    break;
                }
            }
        }

        vm_order = access_order;
    }

    codec::zydis_register inst_regs::get_reg(const uint8_t target) const
    {
        // this would be something like VIP, VSP, VTEMP, etc
        if(target > NUM_OF_VREGS - 1 )
            __debugbreak();

        return vm_order[target];
    }

    std::pair<uint32_t, codec::reg_size> inst_regs::get_stack_displacement(const codec::reg reg) const
    {
        //determine 64bit version of register
        const codec::reg_size reg_size = get_reg_size(reg);
        const codec::reg bit64_reg = get_bit_version(reg, codec::reg_class::gpr_64);

        int found_index = 0;
        constexpr auto reg_count = stack_order.size();

        for(int i = 0; i < reg_count; i++)
        {
            if(bit64_reg == stack_order[i])
            {
                found_index = i;
                break;
            }
        }

        int offset = 0;
        if(reg_size == codec::reg_size::bit_8)
        {
            if(is_upper_8(reg))
                offset = 1;
        }

        return { found_index * 8 + offset, reg_size };
    }

    void inst_regs::enumerate(const std::function<void(codec::zydis_register)>& enumerable, const bool from_back)
    {
        if(from_back)
            std::ranges::for_each(stack_order, enumerable);
        else
            std::for_each(stack_order.rbegin(), stack_order.rend(), enumerable);
    }
}
