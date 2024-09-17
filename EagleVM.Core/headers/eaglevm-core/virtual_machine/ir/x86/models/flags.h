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
}
