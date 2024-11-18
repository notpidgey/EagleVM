#include "eaglevm-core/virtual_machine/machines/owl/register_manager.h"

#include <algorithm>
#include <array>
#include <ranges>

#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/util/random.h"
#include "eaglevm-core/util/assert.h"

namespace eagle::virt::owl
{
    register_manager::register_manager(const settings_ptr& settings_info)
    {
        settings = settings_info;
    }

    void register_manager::init_reg_order()
    {
        // setup stack order
        // insert all xmm registers
        // insert all gpr registers
        {
            for (int i = codec::ymm0; i <= codec::ymm15; i++)
                push_order[i - codec::ymm0] = static_cast<codec::reg>(i);

            for (int i = codec::rax; i <= codec::r15; i++)
                push_order[i - codec::rax + 16] = static_cast<codec::reg>(i);

            // shuffle the stack order
            if (settings->shuffle_push_order)
                std::ranges::shuffle(push_order, util::ran_device::get().gen);
        }

        // setup avail dest registers
        {
            // so we need 64 * 16 bytes for registers
            // reserve maybe 2 ymm registers as temp registers (1 should be more than enough)
            // then we use the rest of the registers as stack memory
            // (16 * 256) - (64 * 16) - (2 * 256) = 2560 avail bits
            // 2560 / 256 = 10 regs for stack

            // save state
            for (int i = 0; i < save_state_ymm.size(); i++)
                save_state_ymm[i] = push_order[push_order.size() - 1 - i];

            // stack
            for (int i = 0; i < stack_ymm.size(); i++)
                stack_ymm[i] = push_order[push_order.size() - 1 - save_state_ymm.size() - i];
        }
    }

    void register_manager::create_mappings()
    {
        std::vector<std::pair<codec::reg, reg_range>> register_points;
        for (auto avail_reg : get_gpr64_regs())
        {
            std::vector<uint16_t> points;
            points.push_back(0); // starting point
            points.push_back(64); // ending point (inclusive)

            constexpr auto num_ranges = 5;
            for (uint16_t i = 0; i < num_ranges - 1; ++i)
            {
                uint16_t point;
                do
                    point = util::ran_device::get().gen_8() % 64; // generates random number between 0 and 63
                while (std::ranges::find(points, point) != points.end());

                points.push_back(point);
            }

            // sort the points
            std::ranges::sort(points);

            // form the inclusive ranges
            std::vector<reg_range> register_ranges;
            for (size_t i = 0; i < points.size() - 1; ++i)
            {
                reg_range reg_range = { points[i], points[i + 1] };
                register_points.emplace_back(avail_reg, reg_range);
            }
        }

        constexpr uint16_t register_point_size = 16 * 64;
        constexpr uint16_t register_mappings_size = 16 * 256;

        // fill in missing bytes we are about to fill into xmm registers
        for (auto i = 0; i < register_mappings_size - register_point_size; i++)
            register_points.emplace_back(codec::reg::none, reg_range{ });

        // shuffle the register ranges
        std::ranges::shuffle(register_points, util::ran_device::get().gen);

        uint32_t current_byte = 0;
        for (auto [src_reg, src_map] : register_points)
        {
            if (src_reg == codec::reg::none)
            {
                // this is a filler register, we just want to increment current byte
                current_byte++;
                continue;
            }

            // legitimate register that we want to map source <-> dest
            // first we want to check if this mapping will cross over the 64 byte boundary of an xmm register
            const uint16_t out_low_qword = current_byte / 64;
            const uint16_t out_high_qword = (current_byte + (src_map.second - src_map.first) - 1) / 64;

            auto occupy_range = [&](const codec::reg reg_dest, const codec::reg reg_src, reg_range& range_src)
            {
                const uint16_t range_size = range_src.second - range_src.first;
                const uint16_t dest_start = current_byte % 256;
                const uint16_t dest_end = dest_start + range_size;

                auto& source_register = source_register_map[reg_src];
                source_register.emplace_back(
                    reg_range{ range_src.first, range_src.second },
                    reg_range{ dest_start, dest_end },
                    reg_dest
                );

                auto& dest_register = dest_register_map[reg_dest];
                dest_register.emplace_back(dest_start, dest_end);

                current_byte += range_size;
            };

            if (out_low_qword == out_high_qword)
            {
                // we are not writing across a boundary so we are fine
                const codec::reg current_register = save_state_ymm[out_low_qword / 4];
                occupy_range(current_register, src_reg, src_map);
            }
            else
            {
                // we have a cross boundary
                const codec::reg first_register = save_state_ymm[out_low_qword / 4];
                const codec::reg last_register = save_state_ymm[out_high_qword / 4];

                const uint16_t dest_midpoint = out_high_qword * 64;
                const uint16_t src_midpoint = src_map.first + (dest_midpoint - current_byte);
                reg_range first_range = { src_map.first, src_midpoint };
                reg_range second_range = { src_midpoint, src_map.second };

                occupy_range(first_register, src_reg, first_range);
                occupy_range(last_register, src_reg, second_range);
            }
        }
    }

    std::pair<uint32_t, codec::reg_size> register_manager::get_stack_displacement(const codec::reg reg) const
    {
        //determine 64bit version of register
        const codec::reg_size reg_size = get_reg_size(reg);
        const codec::reg bit64_reg = get_bit_version(reg, codec::reg_class::gpr_64);

        int found_offset = 0;
        constexpr auto reg_count = push_order.size();

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

    std::vector<codec::reg> register_manager::get_unreserved_temp_xmm() const
    {

    }

    std::array<codec::reg, 16> register_manager::get_gpr64_regs()
    {
        std::array<codec::reg, 16> avail_regs{ };
        for (int i = codec::rax; i <= codec::r15; i++)
            avail_regs[i - codec::rax] = static_cast<codec::reg>(i);

        return avail_regs;
    }

    std::array<codec::reg, 16> register_manager::get_xmm_regs()
    {
        std::array<codec::reg, 16> avail_regs{ };
        for (int i = codec::xmm0; i <= codec::xmm15; i++)
            avail_regs[i - codec::xmm0] = static_cast<codec::reg>(i);

        return avail_regs;
    }

    std::array<codec::reg, 10> register_manager::get_stack_order()
    {

    }

    codec::reg register_manager::get_reserved_temp_xmm(const uint8_t i) const
    {
        VM_ASSERT(i + 1 <= temp_ymm.size(), "attempted to retrieve register with no reservation");
        return temp_ymm[i];
    }
}
