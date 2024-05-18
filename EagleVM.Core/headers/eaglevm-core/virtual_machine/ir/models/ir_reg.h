#pragma once
#include <variant>
#include <string>

#include "eaglevm-core/codec/zydis_enum.h"
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

        vregs,
        vregs_32,
        vregs_16,
        vregs_8,

        vtemp,
        vtemp_32,
        vtemp_16,
        vtemp_8,

        vtemp2,
        vtemp2_32,
        vtemp2_16,
        vtemp2_8,

        vcs,
        vcs_32,
        vcs_16,
        vcs_8,

        vcsret,
        vcsret_32,
        vcsret_16,
        vcsret_8,

        vbase,
    };

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
            case reg_vm::vregs:
            case reg_vm::vregs_32:
            case reg_vm::vregs_16:
            case reg_vm::vregs_8:
                switch (target_size)
                {
                    case ir_size::bit_64:
                        return reg_vm::vregs;
                    case ir_size::bit_32:
                        return reg_vm::vregs_32;
                    case ir_size::bit_16:
                        return reg_vm::vregs_16;
                    case ir_size::bit_8:
                        return reg_vm::vregs_8;
                }
            case reg_vm::vtemp:
            case reg_vm::vtemp_32:
            case reg_vm::vtemp_16:
            case reg_vm::vtemp_8:
                switch (target_size)
                {
                    case ir_size::bit_64:
                        return reg_vm::vtemp;
                    case ir_size::bit_32:
                        return reg_vm::vtemp_32;
                    case ir_size::bit_16:
                        return reg_vm::vtemp_16;
                    case ir_size::bit_8:
                        return reg_vm::vtemp_8;
                }
            case reg_vm::vtemp2:
            case reg_vm::vtemp2_32:
            case reg_vm::vtemp2_16:
            case reg_vm::vtemp2_8:
                switch (target_size)
                {
                    case ir_size::bit_64:
                        return reg_vm::vtemp2;
                    case ir_size::bit_32:
                        return reg_vm::vtemp2_32;
                    case ir_size::bit_16:
                        return reg_vm::vtemp2_16;
                    case ir_size::bit_8:
                        return reg_vm::vtemp2_8;
                }
            case reg_vm::vcs:
            case reg_vm::vcs_32:
            case reg_vm::vcs_16:
            case reg_vm::vcs_8:
                switch (target_size)
                {
                    case ir_size::bit_64:
                        return reg_vm::vcs;
                    case ir_size::bit_32:
                        return reg_vm::vcs_32;
                    case ir_size::bit_16:
                        return reg_vm::vcs_16;
                    case ir_size::bit_8:
                        return reg_vm::vcs_8;
                }
            case reg_vm::vcsret:
            case reg_vm::vcsret_32:
            case reg_vm::vcsret_16:
            case reg_vm::vcsret_8:
                switch (target_size)
                {
                    case ir_size::bit_64:
                        return reg_vm::vcsret;
                    case ir_size::bit_32:
                        return reg_vm::vcsret_32;
                    case ir_size::bit_16:
                        return reg_vm::vcsret_16;
                    case ir_size::bit_8:
                        return reg_vm::vcsret_8;
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
