#include "virtual_machine/vm_register_manager.h"

#include "util/random.h"

void vm_register_manager::init_reg_order()
{
    for (int i = ZYDIS_REGISTER_RAX; i <= ZYDIS_REGISTER_R15; i++)
        reg_stack_order_[i - ZYDIS_REGISTER_RAX] = static_cast<zydis_register>(i);

    std::ranges::shuffle(reg_stack_order_, ran_device::get().gen);
}

std::pair<uint32_t, reg_size> vm_register_manager::get_stack_displacement(const zydis_register reg) const {
    //determine 64bit version of register
    reg_size reg_size = zydis_helper::get_reg_size(reg);

    int found_index = 0;
    for (int i = 0; i < reg_stack_order_.size(); i++)
    {
        if (reg == zydis_helper::get_bit_version(reg_stack_order_[15 - i], reg_size))
        {
            found_index = i;
            break;
        }
    }

    return {(found_index * 8) + (8 - reg_size), reg_size};
}

void vm_register_manager::set_reg_mapping(const short index, const zydis_register reg)
{
    reg_map[index] = reg;
}

bool vm_register_manager::contains_reg_mapping(short mapping, zydis_register reg)
{
    return std::ranges::any_of(reg_map,
        [reg](const zydis_register register_)
            {
                return register_ == reg;
            });
}
