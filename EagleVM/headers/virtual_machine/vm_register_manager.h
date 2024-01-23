#pragma once
#include <array>
#include <algorithm>
#include <vector>
#include <random>

#include "models/vm_defs.h"
#include "util/zydis_defs.h"
#include "util/zydis_helper.h"

class vm_register_manager
{
public:
    std::array<zydis_register, 16> reg_stack_order_;
    std::array<zydis_register, NUM_OF_VREGS> reg_map =
    {
        ZYDIS_REGISTER_RBP, //I_VIP
        ZYDIS_REGISTER_RSI, //I_VSP
        ZYDIS_REGISTER_RDI, //I_VREGS
        ZYDIS_REGISTER_R8,  //I_VTEMP
        ZYDIS_REGISTER_R9   //I_VRET
    };

    void init_reg_order();
    std::pair<uint32_t, reg_size> get_stack_displacement(zydis_register reg);
    void set_reg_mapping(short index, zydis_register reg);
    bool contains_reg_mapping(short mapping, zydis_register reg);
};
