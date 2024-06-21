#include "eaglevm-core/virtual_machine/machines/eagle/register_manager.h"

#include <algorithm>
#include <array>
#include <ranges>

#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/util/random.h"
#include "eaglevm-core/util/assert.h"

namespace eagle::virt::eg
{
    uint8_t register_manager::num_v_regs = 8; // regular VREGS + RSP + RAX
    uint8_t register_manager::num_gpr_regs = 16;

    uint8_t register_manager::index_vip = 0;
    uint8_t register_manager::index_vsp = 1;
    uint8_t register_manager::index_vregs = 2;
    uint8_t register_manager::index_vcs = 3;
    uint8_t register_manager::index_vcsret = 4;
    uint8_t register_manager::index_vbase = 5;

    register_manager::register_manager(const settings_ptr& settings_info)
    {
        settings = settings_info;

        virtual_order_gpr = get_gpr64_regs();

        num_v_temp_reserved = settings->randomize_working_register ? 0 : 3;
        num_v_temp_unreserved = num_gpr_regs - num_v_regs - num_v_temp_reserved;

        num_v_temp_xmm_reserved = 2;
        num_v_temp_xmm_unreserved = 16 - 2;
    }

    void register_manager::init_reg_order()
    {
        // setup stack order
        // insert all xmm registers
        // insert all gpr registers
        {
            for (int i = codec::xmm0; i <= codec::xmm15; i++)
                push_order[i - codec::xmm0] = static_cast<codec::reg>(i);

            for (int i = codec::rax; i <= codec::r15; i++)
                push_order[i - codec::rax + 16] = static_cast<codec::reg>(i);

            // shuffle the stack order
            if (settings->shuffle_push_order)
                std::ranges::shuffle(push_order, util::ran_device::get().gen);
        }

        // setup gpr order
        // insert all gpr registers
        {
            for (int i = codec::rax; i <= codec::r15; i++)
                virtual_order_gpr[i - codec::rax] = static_cast<codec::reg>(i);

            if (settings->shuffle_vm_gpr_order)
                std::ranges::shuffle(virtual_order_gpr, util::ran_device::get().gen);

            // force RSP to be at virtual_order_gpr[num_vregs - 1]
            // force RAX to be at virtual_order_gpr[num_vregs - 2]

            // find the positions of RSP and RAX in the vector
            auto it_rsp = std::ranges::find(virtual_order_gpr, codec::rsp);
            auto it_rax = std::ranges::find(virtual_order_gpr, codec::rax);

            // check if RSP and RAX were found in the vector
            if (it_rsp != virtual_order_gpr.end() && it_rax != virtual_order_gpr.end())
            {
                // swap RSP with the element at position num_vregs
                std::iter_swap(it_rsp, virtual_order_gpr.begin() + num_v_regs - 1);

                if (it_rax == virtual_order_gpr.begin() + num_v_regs - 1)
                    it_rax = it_rsp;

                // swap RAX with the element at position num_vregs + 1
                std::iter_swap(it_rax, virtual_order_gpr.begin() + num_v_regs - 2);
            }
        }

        // setup xmm order
        // insert all xmm registers
        {
            for (int i = codec::xmm0; i <= codec::xmm31; i++)
                virtual_order_xmm[i - codec::xmm0] = static_cast<codec::reg>(i);

            if (settings->shuffle_vm_xmm_order)
                std::ranges::shuffle(virtual_order_xmm, util::ran_device::get().gen);
        }

        // setup avail dest registers
        {
            // xmm all avail
            for (int i = 0; i < num_v_temp_xmm_unreserved; i++)
                dest_register_map[virtual_order_xmm[virtual_order_xmm.size() - 1 - i]] = { };
        }
    }

    void register_manager::create_mappings()
    {
        const std::array<codec::reg, 16> avail_regs = get_gpr64_regs();
        for (auto avail_reg : avail_regs)
        {
            std::vector<uint16_t> points;
            points.push_back(0); // starting point
            points.push_back(64); // ending point (inclusive)

            constexpr auto numRanges = 64;
            for (uint16_t i = 0; i < numRanges - 1; ++i)
            {
                uint16_t point;
                do
                {
                    point = util::ran_device::get().gen_8() % 64; // generates random number between 0 and 63
                }
                while (std::ranges::find(points, point) != points.end());
                points.push_back(point);
            }

            // sort the points
            std::ranges::sort(points);

            // form the inclusive ranges
            std::vector<reg_range> register_ranges;
            for (size_t i = 0; i < points.size() - 1; ++i)
                register_ranges.emplace_back(points[i], points[i + 1]);

            for (const auto& [first, last] : register_ranges)
            {
                const auto length = last - first;

                auto find_avail_range = [](
                    const std::vector<reg_range>& occupied_ranges,
                    const uint16_t range_length,
                    const uint16_t max_bit
                ) -> std::optional<reg_range>
                {
                    std::vector<uint16_t> potential_starts;
                    for (uint16_t start = 0; start <= max_bit - range_length; ++start)
                        potential_starts.push_back(start);

                    // Shuffle the potential start points to randomize the search
                    std::ranges::shuffle(potential_starts, util::ran_device::get().gen);

                    for (const uint16_t start : potential_starts)
                    {
                        bool available = true;
                        for (const auto& [fst, snd] : occupied_ranges)
                        {
                            if (!(start + range_length <= fst || start >= snd))
                            {
                                available = false;
                                break;
                            }
                        }

                        if (available)
                        {
                            return reg_range{ start, start + range_length };
                        }
                    }

                    return std::nullopt;
                };

                // Shuffle the keys in dest_register_map to iterate in random order
                std::vector<codec::reg> dest_regs;

                for (const auto& reg : dest_register_map | std::views::keys) dest_regs.push_back(reg);
                std::ranges::shuffle(dest_regs, util::ran_device::get().gen);

                bool range_mapped = false;
                for (const auto& dest_reg : dest_regs)
                {
                    auto& occ_ranges = dest_register_map[dest_reg];
                    if (std::optional<reg_range> found_range = find_avail_range(occ_ranges, length, get_reg_size(dest_reg)))
                    {
                        // insert into source registers
                        std::vector<reg_mapped_range>& source_register = source_register_map[avail_reg];
                        if (get_reg_class(dest_reg) == codec::xmm_128)
                        {
                            // check for cross boundary
                            auto [from, to] = found_range.value();
                            if (from <= 63 && to > 64)
                            {
                                // write across boundary
                                // this means we need to create two different ones

                                source_register.push_back({ { first, first + (64 - from) }, { from, 64 }, dest_reg });
                                source_register.push_back({ { first + (64 - from), last }, { 64, to }, dest_reg });
                            }
                            else
                            {
                                source_register.push_back({ { first, last }, found_range.value(), dest_reg });
                            }
                        }
                        else
                        {
                            source_register.push_back({ { first, last }, found_range.value(), dest_reg });
                        }

                        occ_ranges.push_back(found_range.value());

                        range_mapped = true;
                        break; // exit loop once a valid range is found
                    }
                }

                VM_ASSERT(range_mapped, "unable to find valid range to map registers");
            }
        }
    }

    std::pair<uint32_t, codec::reg_size> register_manager::get_stack_displacement(const codec::reg reg) const
    {
        //determine 64bit version of register
        const codec::reg_size reg_size = get_reg_size(reg);
        const codec::reg bit64_reg = get_bit_version(reg, codec::reg_class::gpr_64);

        int found_offset = 0;
        const auto reg_count = push_order.size();

        for (int i = 0; i < reg_count; i++)
        {
            if (bit64_reg == push_order[i])
                break;

            found_offset += get_reg_size(push_order[i]) / 8;
        }

        int offset = 0;
        if (reg_size == codec::reg_size::bit_8)
        {
            if (is_upper_8(reg))
                offset = 1;
        }

        return { found_offset + offset, reg_size };
    }

    std::vector<reg_mapped_range> register_manager::get_register_mapped_ranges(const codec::reg reg)
    {
        return source_register_map[reg];
    }

    std::vector<reg_range> register_manager::get_occupied_ranges(const codec::reg reg)
    {
        return dest_register_map[reg];
    }

    std::vector<reg_range> register_manager::get_unoccupied_ranges(const codec::reg reg)
    {
        const uint16_t bit_count = static_cast<uint16_t>(get_reg_size(reg));
        std::vector<reg_range> occupied_ranges = dest_register_map[reg];
        std::vector<reg_range> unoccupied_ranges;

        // Sort the occupied ranges by their starting point
        std::ranges::sort(occupied_ranges);

        unsigned int current_pos = 0;

        for (const auto& [fst, snd] : occupied_ranges)
        {
            if (fst > current_pos)
            {
                // there is a gap before this range
                unoccupied_ranges.emplace_back(current_pos, fst - 1);
            }

            // move the current position past this range
            current_pos = snd + 1;
        }

        // check if there is an unoccupied range after the last occupied range
        if (current_pos < bit_count)
            unoccupied_ranges.emplace_back(current_pos, bit_count - 1);

        return unoccupied_ranges;
    }

    codec::reg register_manager::get_vm_reg(const uint8_t i) const
    {
        VM_ASSERT(i + 1 <= num_v_regs, "attempted to access vreg outside of boundaries");
        return virtual_order_gpr[i];
    }

    std::vector<codec::reg> register_manager::get_unreserved_temp() const
    {
        std::vector<codec::reg> out;
        for (uint8_t i = 0; i < num_v_temp_unreserved; i++)
            out.push_back(virtual_order_gpr[virtual_order_gpr.size() - 1 - i]);

        return out;
    }

    codec::reg register_manager::get_reserved_temp(const uint8_t i) const
    {
        VM_ASSERT(i + 1 <= num_v_temp_reserved, "attempted to retreive register with no reservation");
        return virtual_order_gpr[num_v_regs + i];
    }

    std::array<codec::reg, 16> register_manager::get_gpr64_regs()
    {
        std::array<codec::reg, 16> avail_regs{ };
        for (int i = codec::rax; i <= codec::r15; i++)
            avail_regs[i - codec::rax] = static_cast<codec::reg>(i);

        return avail_regs;
    }

    std::array<codec::reg, 32> register_manager::get_xmm_regs()
    {
        std::array<codec::reg, 32> avail_regs{ };
        for (int i = codec::xmm0; i <= codec::xmm31; i++)
            avail_regs[i - codec::xmm0] = static_cast<codec::reg>(i);

        return avail_regs;
    }

    codec::reg register_manager::get_reserved_temp_xmm(uint8_t i) const
    {
        VM_ASSERT(i + 1 <= num_v_temp_xmm_reserved, "attempted to retreive register with no reservation");
        return virtual_order_xmm[i];
    }
}
