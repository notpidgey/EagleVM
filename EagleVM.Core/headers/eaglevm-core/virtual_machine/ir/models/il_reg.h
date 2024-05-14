#pragma once
#include <variant>
#include <string>

#include "eaglevm-core/virtual_machine/ir/models/il_size.h"

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

    inline reg_vm get_bit_version(const reg_vm vm_reg, const il_size target_size)
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
                    case il_size::bit_64:
                        return reg_vm::vip;
                    case il_size::bit_32:
                        return reg_vm::vip_32;
                    case il_size::bit_16:
                        return reg_vm::vip_16;
                    case il_size::bit_8:
                        return reg_vm::vip_8;
                }
            case reg_vm::vsp:
            case reg_vm::vsp_32:
            case reg_vm::vsp_16:
            case reg_vm::vsp_8:
                switch (target_size)
                {
                    case il_size::bit_64:
                        return reg_vm::vsp;
                    case il_size::bit_32:
                        return reg_vm::vsp_32;
                    case il_size::bit_16:
                        return reg_vm::vsp_16;
                    case il_size::bit_8:
                        return reg_vm::vsp_8;
                }
            case reg_vm::vregs:
            case reg_vm::vregs_32:
            case reg_vm::vregs_16:
            case reg_vm::vregs_8:
                switch (target_size)
                {
                    case il_size::bit_64:
                        return reg_vm::vregs;
                    case il_size::bit_32:
                        return reg_vm::vregs_32;
                    case il_size::bit_16:
                        return reg_vm::vregs_16;
                    case il_size::bit_8:
                        return reg_vm::vregs_8;
                }
            case reg_vm::vtemp:
            case reg_vm::vtemp_32:
            case reg_vm::vtemp_16:
            case reg_vm::vtemp_8:
                switch (target_size)
                {
                    case il_size::bit_64:
                        return reg_vm::vtemp;
                    case il_size::bit_32:
                        return reg_vm::vtemp_32;
                    case il_size::bit_16:
                        return reg_vm::vtemp_16;
                    case il_size::bit_8:
                        return reg_vm::vtemp_8;
                }
            case reg_vm::vtemp2:
            case reg_vm::vtemp2_32:
            case reg_vm::vtemp2_16:
            case reg_vm::vtemp2_8:
                switch (target_size)
                {
                    case il_size::bit_64:
                        return reg_vm::vtemp2;
                    case il_size::bit_32:
                        return reg_vm::vtemp2_32;
                    case il_size::bit_16:
                        return reg_vm::vtemp2_16;
                    case il_size::bit_8:
                        return reg_vm::vtemp2_8;
                }
            case reg_vm::vcs:
            case reg_vm::vcs_32:
            case reg_vm::vcs_16:
            case reg_vm::vcs_8:
                switch (target_size)
                {
                    case il_size::bit_64:
                        return reg_vm::vcs;
                    case il_size::bit_32:
                        return reg_vm::vcs_32;
                    case il_size::bit_16:
                        return reg_vm::vcs_16;
                    case il_size::bit_8:
                        return reg_vm::vcs_8;
                }
            case reg_vm::vcsret:
            case reg_vm::vcsret_32:
            case reg_vm::vcsret_16:
            case reg_vm::vcsret_8:
                switch (target_size)
                {
                    case il_size::bit_64:
                        return reg_vm::vcsret;
                    case il_size::bit_32:
                        return reg_vm::vcsret_32;
                    case il_size::bit_16:
                        return reg_vm::vcsret_16;
                    case il_size::bit_8:
                        return reg_vm::vcsret_8;
                }
            case reg_vm::vbase:
                switch (target_size)
                {
                    case il_size::bit_64:
                        return reg_vm::vbase;
                }
            case reg_vm::none:
            default:
                return reg_vm::none;
        }
    }

    enum class size
    {
        qword,
        dword,
        word,
        byte,
    };

    using reg_v = std::variant<reg_vm, codec::reg>;

    inline bool is_vm_reg(reg_v reg)
    {
        bool result;
        std::visit([&result](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, reg_vm>)
                result = true;
            else
                result = false;
        }, reg);

        return result;
    }

    inline bool is_x86_reg(reg_v reg)
    {
        bool result;
        std::visit([&result](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, asmb::x86::reg>)
                result = true;
            else
                result = false;
        }, reg);

        return result;
    }

    inline reg_type get_reg_type(const reg_v reg)
    {
        if (is_x86_reg(reg))
            return reg_type::x86;

        if (is_vm_reg(reg))
            return reg_type::vm;

        return reg_type::invalid;
    }

    inline std::string reg_to_string(reg_v reg)
    {
        std::string result;
        std::visit([&result](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, reg_vm>)
            {
                switch (arg)
                {
                    case reg_vm::vip: result = "vip";
                        break;
                    case reg_vm::vsp: result = "vsp";
                        break;
                    case reg_vm::vregs: result = "vregs";
                        break;
                    case reg_vm::vtemp: result = "vtemp";
                        break;
                    case reg_vm::vtemp2: result = "vtemp2";
                        break;
                    case reg_vm::vcs: result = "vcs";
                        break;
                    case reg_vm::vcsret: result = "vcsret";
                        break;
                    case reg_vm::vbase: result = "vbase";
                        break;
                }
            }
            else if constexpr (std::is_same_v<T, reg_x86>)
            {
                switch (arg)
                {
                    case reg_x86::rax: result = "rax";
                        break;
                    case reg_x86::rbx: result = "rbx";
                        break;
                    case reg_x86::rcx: result = "rcx";
                        break;
                    case reg_x86::rdx: result = "rdx";
                        break;
                    case reg_x86::rbp: result = "rbp";
                        break;
                    case reg_x86::rsp: result = "rsp";
                        break;
                    case reg_x86::rsi: result = "rsi";
                        break;
                    case reg_x86::rdi: result = "rdi";
                        break;
                    case reg_x86::r8: result = "r8";
                        break;
                    case reg_x86::r9: result = "r9";
                        break;
                    case reg_x86::r10: result = "r10";
                        break;
                    case reg_x86::r11: result = "r11";
                        break;
                    case reg_x86::r12: result = "r12";
                        break;
                    case reg_x86::r13: result = "r13";
                        break;
                    case reg_x86::r14: result = "r14";
                        break;
                    case reg_x86::r15: result = "r15";
                        break;
                }
            }
        }, reg);

        return result;
    }
}
