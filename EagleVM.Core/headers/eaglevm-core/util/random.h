#pragma once
#include <random>
#include "eaglevm-core/util/assert.h"

namespace eagle::util
{
    class ran_device
    {
    public:
        std::mt19937 gen{ };
        uint64_t seed;

        ran_device();
        static ran_device& get();

        uint64_t gen_64();
        uint32_t gen_32();
        uint16_t gen_16();
        uint8_t gen_8();
        uint64_t gen_dist(std::uniform_int_distribution<uint64_t>& distribution);
        bool gen_chance(float chance);
        double gen_dist(std::uniform_real_distribution<>& distribution);

        template <typename T>
        const T& random_elem(const std::vector<T>& vec)
        {
            VM_ASSERT(!vec.empty(), "cannot get a random element from an empty vector");

            std::uniform_int_distribution<> dist(0, vec.size() - 1);
            return vec[dist(gen)];
        }

        ran_device(const ran_device&) = delete;
        ran_device& operator=(const ran_device&) = delete;
    };

    ran_device& get_ran_device();
}
