#pragma once
#include <random>

namespace eagle::util
{
    class ran_device
    {
    public:
        std::mt19937 gen{};

        ran_device();
        static ran_device& get();

        uint64_t gen_64();
        uint32_t gen_32();
        uint16_t gen_16();
        uint8_t gen_8();
        uint64_t gen_dist(std::uniform_int_distribution<uint64_t>& distribution);
        double gen_dist(std::uniform_real_distribution<>& distribution);

        ran_device(const ran_device&) = delete;
        ran_device& operator=(const ran_device&) = delete;
    };
}
