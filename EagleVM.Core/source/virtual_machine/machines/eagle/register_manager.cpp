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
            for (int i = codec::xmm0; i <= codec::xmm15; i++)
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
        std::vector<std::pair<codec::reg, reg_range>> register_points;
        for (auto avail_reg : get_gpr64_regs())
        {
            std::vector<uint16_t> points;
            points.push_back(0); // starting point
            points.push_back(64); // ending point (inclusive)

            constexpr auto num_ranges = 2;
            //  TODO: remove all these comments?
            //for (uint16_t i = 0; i < num_ranges - 1; ++i)
            //{
            //    uint16_t point;
            //    do
            //        point = util::ran_device::get().gen_8() % 64; // generates random number between 0 and 63
            //    while (std::ranges::find(points, point) != points.end());
//
            //    points.push_back(point);
            //}
//
            //// sort the points
            //std::ranges::sort(points);
//
            //// form the inclusive ranges
            //std::vector<reg_range> register_ranges;
            //for (size_t i = 0; i < points.size() - 1; ++i)
            //{
            //    reg_range reg_range = { points[i], points[i + 1] };
            //    register_points.emplace_back(avail_reg, reg_range);
            //}

            register_points.emplace_back(avail_reg, reg_range{0, 32});
            register_points.emplace_back(avail_reg, reg_range{32, 64});
        }

        constexpr uint16_t register_point_size = 16 * 64;
        constexpr uint16_t register_mappings_size = 16 * 128;

        // fill in missing bytes we are about to fill into xmm registers
        for (auto i = 0; i < register_mappings_size - register_point_size; i++)
            register_points.emplace_back(codec::reg::none, reg_range{ });

        // shuffle the register ranges
        // std::ranges::shuffle(register_points, util::ran_device::get().gen);

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
            const uint16_t xmm_low = current_byte / 64;
            const uint16_t xmm_high = (current_byte + (src_map.second - src_map.first) - 1) / 64;

            auto occupy_range = [&](codec::reg reg_dest, codec::reg reg_src, reg_range& range_src)
            {
                const uint16_t range_size = range_src.second - range_src.first;
                const uint16_t dest_start = current_byte % 128;
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

            if (xmm_low == xmm_high)
            {
                // we are not writing across a boundary so we are fine
                const codec::reg current_register = static_cast<codec::reg>(codec::xmm0 + xmm_low / 2);
                occupy_range(current_register, src_reg, src_map);
            }
            else
            {
                // we have a cross boundary
                const codec::reg first_register = static_cast<codec::reg>(codec::xmm0 + xmm_low / 2);
                const codec::reg last_register = static_cast<codec::reg>(codec::xmm0 + xmm_high / 2);

                const uint16_t dest_midpoint = xmm_high * 64;
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

    std::vector<codec::reg> register_manager::get_unreserved_temp_xmm() const
    {
        std::vector<codec::reg> out;
        for (uint8_t i = 0; i < num_v_temp_xmm_unreserved; i++)
            out.push_back(virtual_order_xmm[virtual_order_gpr.size() - 1 - i]);

        return out;
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

    codec::reg register_manager::get_reserved_temp_xmm(uint8_t i) const
    {
        VM_ASSERT(i + 1 <= num_v_temp_xmm_reserved, "attempted to retreive register with no reservation");
        return virtual_order_xmm[i];
    }
}
