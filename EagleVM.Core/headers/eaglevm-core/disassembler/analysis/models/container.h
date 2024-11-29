#pragma once
#include <cstdint>

namespace eagle::dasm::analysis
{
    /*
     * reg_set_container is a class which is used for representing the current liveness of a register
     * TBits represents the amount of bits there are in the register
     * TCount represents the amount of registers there are
     * DBase represents the base register of the largest enclosing register (R0, ZMM0)
     *
     * when working with rflags, the container can be represented as reg_set_container<64 * 8, 1>
     * since only bytes are significant, we have to treat each bit of rflags as a byte (hence 64 * 8)
     */
    template <uint16_t TBits, uint16_t TCount, uint16_t DBase = ZYDIS_REGISTER_NONE>
    class reg_set_container
    {
    public:
        friend reg_set_container operator|(const reg_set_container& regs, const reg_set_container& other)
        {
            reg_set_container container;
            for (int i = 0; i < get_size(); i++)
                container.register_state[i] = regs.register_state[i] | other.register_state[i];

            return container;
        }

        friend reg_set_container operator-(const reg_set_container& regs, const reg_set_container& other)
        {
            reg_set_container container;
            for (int i = 0; i < get_size(); i++)
                container.register_state[i] = regs.register_state[i] & ~other.register_state[i];

            return container;
        }

        friend reg_set_container operator&(const reg_set_container& regs, const reg_set_container& other)
        {
            reg_set_container container;
            for (int i = 0; i < get_size(); i++)
                container.register_state[i] = regs.register_state[i] & other.register_state[i];

            return container;
        }

        reg_set_container& operator|=(const reg_set_container& regs)
        {
            for (int i = 0; i < get_size(); i++)
                register_state[i] |= regs.register_state[i];

            return *this;
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

        /*
         * after a register is updated, to reflect the update you can call `insert` which will enumerate each byte of the register
         * the presence of each byte will be saved. this means each bit of the register is NOT accounted for. only bytes.
         */
        bool insert(const codec::reg reg)
        {
            const auto enclosing = get_bit_version(reg, static_cast<codec::reg_size>(TBits));
            const auto register_index = static_cast<uint16_t>(enclosing) - DBase;

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

        bool insert_byte(const uint16_t byte_idx)
        {
            uint64_t mask = 1ull << byte_idx % 64;

            const bool exists = register_state[byte_idx / 64] & mask;
            register_state[byte_idx / 64] |= mask;

            return exists;
        }


    private:
        static constexpr uint16_t get_size()
        {
            constexpr auto val = TCount * TBits / 8 / 64.0;
            return static_cast<float>(static_cast<uint16_t>(val)) == val
                       ? static_cast<uint16_t>(val)
                       : static_cast<uint16_t>(val) + (val > 0 ? 1 : 0);
        }

    public:
        uint64_t register_state[get_size()] = { };
    };
}
