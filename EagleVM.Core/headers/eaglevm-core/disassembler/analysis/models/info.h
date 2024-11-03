#pragma once
#include "eaglevm-core/disassembler/analysis/models/container.h"
#include "eaglevm-core/util/assert.h"

namespace eagle::dasm::analysis
{
    using reg64_set = reg_set_container<64, 16, ZYDIS_REGISTER_RAX>;
    using reg512_set = reg_set_container<512, 16, ZYDIS_REGISTER_ZMM0>;
    using eflags_set = reg_set_container<32 * 8, 1>;

    class liveness_info
    {
    public:
        bool insert_register(const codec::reg reg)
        {
            const codec::reg largest_encoding = get_largest_enclosing(reg);
            const codec::reg_class largest_class = get_reg_class(largest_encoding);

            bool ret = false;
            if (largest_class == ZYDIS_REGCLASS_ZMM)
                ret = r512.insert(reg);
            else if (largest_class == ZYDIS_REGCLASS_GPR64)
                ret = r64.insert(reg);
            else if (largest_class == ZYDIS_REGISTER_RFLAGS)
                ret = flags.insert(reg);
            else
            VM_ASSERT("unknown regclass found");

            return ret;
        }

        void insert_flags(const uint64_t data)
        {
            for (int i = 0; i < 64; i++)
                if (data >> i & 1)
                    flags.insert_byte(i);
        }

        friend liveness_info& operator-(const liveness_info& first, const liveness_info& second)
        {
            liveness_info info;
            info.r64 = first.r64 - second.r64;
            info.r512 = first.r512 - second.r512;
            info.flags = first.flags - second.flags;

            return info;
        }

        friend liveness_info& operator|(const liveness_info& first, const liveness_info& second)
        {
            liveness_info info;
            info.r64 = first.r64 | second.r64;
            info.r512 = first.r512 | second.r512;
            info.flags = first.flags | second.flags;

            return info;
        }

        friend liveness_info operator&(const liveness_info& first, const liveness_info& second)
        {
            liveness_info info;
            info.r64 = first.r64 & second.r64;
            info.r512 = first.r512 & second.r512;
            info.flags = first.flags & second.flags;

            return info;
        }

        liveness_info& operator|=(const liveness_info& first)
        {
            this->r64 |= first.r64;
            this->r512 |= first.r512;
            this->flags |= first.flags;

            return *this;
        }

        bool operator==(const liveness_info& other) const
        {
            return this->r64 == other.r64 && this->r512 == other.r512 && this->flags == other.flags;
        }

        [[nodiscard]] uint8_t get_gpr64(const codec::reg reg) const
        {
            const auto idx = reg - ZYDIS_REGISTER_RAX;
            return r64.register_state[idx / 8] >> idx * 8;
        }

        [[nodiscard]] uint8_t get_zmm512(const codec::reg reg) const
        {
            const auto idx = reg - ZYDIS_REGISTER_ZMM0;
            return r512.register_state[idx / 8] >> idx * 8;
        }

        uint64_t get_flags() const
        {
            return flags.register_state[0];
        }

    private:
        reg64_set r64;
        reg512_set r512;
        eflags_set flags;
    };
}
