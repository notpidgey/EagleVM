#pragma once
#include "eaglevm-core/disassembler/analysis/models/container.h"
#include "eaglevm-core/util/assert.h"

namespace eagle::dasm::analysis
{
    using reg64_set = reg_set_container<64, 16, ZYDIS_REGISTER_RAX>;
    using reg512_set = reg_set_container<512, 16, ZYDIS_REGISTER_ZMM0>;

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
            else
                VM_ASSERT("unknown regclass found");

            return ret;
        }

        liveness_info& operator-(const liveness_info& def_set)
        {
            this->r64 -= def_set.r64;
            this->r512 -= def_set.r512;
            return *this;
        }

        liveness_info& operator|(const liveness_info& diff)
        {
            this->r64 |= diff.r64;
            this->r512 |= diff.r512;
            return *this;
        }

        liveness_info& operator|=(const liveness_info& diff)
        {
            this->r64 |= diff.r64;
            this->r512 |= diff.r512;
            return *this;
        }

        bool operator==(const liveness_info& second) const
        {
            return this->r64 == second.r64 && this->r512 == second.r512;
        }

        [[nodiscard]] uint8_t get_gpr64(const codec::reg reg) const
        {
            const auto idx = reg - ZYDIS_REGISTER_RAX;
            return r64.register_state[idx / 8] >> idx * 8;
        }

    private:
        reg64_set r64;
        reg512_set r512;
    };
}
