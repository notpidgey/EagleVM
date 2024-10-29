#pragma once
#include "eaglevm-core/virtual_machine/ir/models/ir_size.h"

namespace eagle::ir
{
    enum class reg_type
    {
        invalid,
        x86,
        vm
    };

    enum class reg_vm
    {
        none,

        vip,
        vip_32,
        vip_16,
        vip_8,

        vsp,
        vsp_32,
        vsp_16,
        vsp_8,

        vbase,
    };

    inline std::string reg_vm_to_string(const reg_vm reg)
    {
        switch (reg)
        {
            case reg_vm::none:
                return "none";
            case reg_vm::vip:
                return "vip";
            case reg_vm::vip_32:
                return "vip_32";
            case reg_vm::vip_16:
                return "vip_16";
            case reg_vm::vip_8:
                return "vip_8";
            case reg_vm::vsp:
                return "vsp";
            case reg_vm::vsp_32:
                return "vsp_32";
            case reg_vm::vsp_16:
                return "vsp_16";
            case reg_vm::vsp_8:
                return "vsp_8";
            case reg_vm::vbase:
                return "vbase";
            default:
                return "unknown";
        }
    }

    inline reg_vm get_bit_version(const reg_vm vm_reg, const ir_size target_size)
    {
        // yes i know there is cleaner ways of doing it
        // idc
        switch (vm_reg)
        {
            case reg_vm::vip:
            case reg_vm::vip_32:
            case reg_vm::vip_16:
            case reg_vm::vip_8:
                switch (target_size)
                {
                    case ir_size::bit_64:
                        return reg_vm::vip;
                    case ir_size::bit_32:
                        return reg_vm::vip_32;
                    case ir_size::bit_16:
                        return reg_vm::vip_16;
                    case ir_size::bit_8:
                        return reg_vm::vip_8;
                }
            case reg_vm::vsp:
            case reg_vm::vsp_32:
            case reg_vm::vsp_16:
            case reg_vm::vsp_8:
                switch (target_size)
                {
                    case ir_size::bit_64:
                        return reg_vm::vsp;
                    case ir_size::bit_32:
                        return reg_vm::vsp_32;
                    case ir_size::bit_16:
                        return reg_vm::vsp_16;
                    case ir_size::bit_8:
                        return reg_vm::vsp_8;
                }
            case reg_vm::vbase:
                switch (target_size)
                {
                    case ir_size::bit_64:
                        return reg_vm::vbase;
                }
            case reg_vm::none:
            default:
                return reg_vm::none;
        }
    }
}
