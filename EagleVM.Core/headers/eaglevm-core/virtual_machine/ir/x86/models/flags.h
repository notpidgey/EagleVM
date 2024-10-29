#pragma once

namespace eagle::ir
{
    enum x86_cpu_flag : uint32_t
    {
        NONE = 0,
        CF = 1ul << 0,
        PF = 1ul << 2,
        AF = 1ul << 4,
        ZF = 1ul << 6,
        SF = 1ul << 7,
        TF = 1ul << 8,
        IF = 1ul << 9,
        DF = 1ul << 10,
        OF = 1ul << 11,
        IOPL = 1ul << 12,
        NT = 1ul << 14,
        RF = 1ul << 16,
        VM = 1ul << 17,
        AC = 1ul << 18,
        VIF = 1ul << 19,
        VIP = 1ul << 20,
        ID = 1ul << 21
    };

    inline std::string x86_cpu_flag_to_string(const x86_cpu_flag flags)
    {
        if (flags == NONE)
        {
            return "NONE";
        }

        std::vector<std::string> flag_names;

        if (flags & CF) flag_names.push_back("CF");
        if (flags & PF) flag_names.push_back("PF");
        if (flags & AF) flag_names.push_back("AF");
        if (flags & ZF) flag_names.push_back("ZF");
        if (flags & SF) flag_names.push_back("SF");
        if (flags & TF) flag_names.push_back("TF");
        if (flags & IF) flag_names.push_back("IF");
        if (flags & DF) flag_names.push_back("DF");
        if (flags & OF) flag_names.push_back("OF");
        if (flags & IOPL) flag_names.push_back("IOPL");
        if (flags & NT) flag_names.push_back("NT");
        if (flags & RF) flag_names.push_back("RF");
        if (flags & VM) flag_names.push_back("VM");
        if (flags & AC) flag_names.push_back("AC");
        if (flags & VIF) flag_names.push_back("VIF");
        if (flags & VIP) flag_names.push_back("VIP");
        if (flags & ID) flag_names.push_back("ID");

        std::string result;
        for (size_t i = 0; i < flag_names.size(); ++i)
        {
            result += flag_names[i];
            if (i < flag_names.size() - 1)
            {
                result += " | ";
            }
        }

        return result;
    }
}
