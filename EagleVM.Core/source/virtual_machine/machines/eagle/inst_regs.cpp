#include "eaglevm-core/virtual_machine/machines/eagle/inst_regs.h"

#include <algorithm>
#include <array>

#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/util/random.h"

namespace eagle::virt::eg
{
    inst_regs::inst_regs()
    {
        for (int i = codec::rax; i <= codec::r15; i++)
            dest_register_map.push_back({ static_cast<codec::reg>(i), { } });

        for (int i = codec::xmm0; i <= codec::xmm15; i++)
            dest_register_map.push_back({ static_cast<codec::reg>(i), { } });
    }

    void inst_regs::create_mappings()
    {
        std::array<codec::reg, 16> avail_regs{ };
        for (int i = codec::rax; i <= codec::r15; i++)
            avail_regs[i - codec::rax] = static_cast<codec::reg>(i);

        for (auto i = 0; i < avail_regs.size(); i++)
        {
            std::vector<uint64_t> points;
            points.push_back(0);
            points.push_back(64);

            for (uint64_t j = 0; j < 5 - 1; ++j)
            {
                uint64_t point;
                do
                    point = util::ran_device::get().gen_64() % 64;
                while (std::ranges::find(points, point) != points.end());

                points.push_back(point);
            }

            std::ranges::sort(points);

            std::vector<reg_range> register_ranges;
            for (size_t j = 0; j < points.size() - 1; ++j)
                register_ranges.emplace_back(points[j], points[j + 1]);

            for (auto& [first, last] : register_ranges)
            {
                const uint16_t length = last - first;

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

                std::uniform_int_distribution<uint64_t> distr(0, dest_register_map.size() - 1);
                const uint64_t random_dest_idx = util::ran_device::get().gen_dist(distr);

                auto& [dest_reg, occ_ranges] = dest_register_map[random_dest_idx];

                // give 10 attempts to map the range
                std::optional<reg_range> found_range = std::nullopt;
                for (auto j = 0; j < 10 && found_range == std::nullopt; j++)
                    found_range = find_avail_range(occ_ranges, length, get_reg_size(dest_reg));

                assert(found_range.has_value(), "unable to find valid range to map registers");
                occ_ranges.push_back(found_range.value());
            }
        }
    }
}
