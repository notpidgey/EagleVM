#pragma once
#include <cstdint>
#include "eaglevm-core/disassembler/disassembler.h"

namespace eagle::dasm::analysis
{
    template <uint16_t TBits, uint16_t TCount, uint16_t DBase>
    class reg_set_container
    {
    public:
        friend reg_set_container& operator|(const reg_set_container& regs, const reg_set_container& other)
        {
            reg_set_container container;
            for (int i = 0; i < get_size(); i++)
                container.register_state[i] = regs.register_state[i] | other.register_state[i];

            return container;
        }

        reg_set_container& operator|=(const reg_set_container& regs)
        {
            for (int i = 0; i < get_size(); i++)
                register_state[i] |= regs.register_state[i];

            return *this;
        }

        friend reg_set_container& operator-(const reg_set_container& regs, const reg_set_container& other)
        {
            reg_set_container container;
            for (int i = 0; i < get_size(); i++)
                container.register_state[i] = regs.register_state[i] & ~other.register_state[i];

            return container;
        }

        reg_set_container& operator-=(const reg_set_container& regs)
        {
            for (int i = 0; i < get_size(); i++)
                register_state[i] &= ~regs.register_state[i];

            return *this;
        }

        bool operator==(const reg_set_container& regs) const
        {
            for (int i = 0; i < get_size(); i++)
                if (register_state[i] != regs.register_state[i])
                    return false;
            return true;
        }

        bool insert(const codec::reg reg)
        {
            const auto enclosing = get_bit_version(reg, static_cast<codec::reg_size>(TBits));
            const auto register_index = static_cast<uint16_t>(enclosing) - (uint16_t)DBase;

            auto byte_index = (TBits * register_index) / 8;
            if (is_upper_8(reg)) byte_index += 1;

            bool exists = true;
            auto byte_count = static_cast<uint16_t>(get_reg_size(reg)) / 8;
            for (auto byte = 0; byte < byte_count; byte++)
            {
                const auto idx = (byte_index + byte) / 64;
                const auto bit = (byte_index + byte) % 64;
                exists = register_state[idx] & 1ull << bit;

                register_state[idx] |= 1ull << bit;
            }

            return !exists;
        }

        uint64_t register_state[(TBits * TCount / 8) / 64] = { };

    private:
        static constexpr uint16_t get_size()
        {
            return (TBits * TCount / 8) / 64;
        }
    };
}