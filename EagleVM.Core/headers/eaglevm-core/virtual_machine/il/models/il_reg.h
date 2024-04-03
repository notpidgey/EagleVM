#pragma once
#include <variant>
#include <string>

#include "eaglevm-core/assembler/x86/zydis_defs.h"
#include "eaglevm-core/assembler/x86/zydis_enum.h"

namespace eagle::il
{
    enum class reg_type
    {
        invalid,
        x86,
        vm
    };

    std::string reg_type_to_string(reg_type reg)
    {
        switch (reg)
        {
            case reg_type::x86:
                return "x86";
            case reg_type::vm:
                return "vm";
        }
    }

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
    enum class size
    {
        qword,
        dword,
        word,
        byte,
    };


    using reg_v = std::variant<reg_vm, asmbl::x86::reg>;

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
            if constexpr (std::is_same_v<T, asmbl::x86::reg>)
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
