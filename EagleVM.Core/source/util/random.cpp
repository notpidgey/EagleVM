#include "eaglevm-core/util/random.h"

namespace eagle::util
{
    ran_device& get_ran_device()
    {
        return ran_device::get();
    }

    ran_device::ran_device()
    {
#ifdef _DEBUG
        seed = 0xDEADBEEF;
        gen.seed(0xDEADBEEF);
#else
        std::random_device rd{};
        seed = rd();
        gen.seed(seed);
#endif
    }

    ran_device& ran_device::get()
    {
        static ran_device instance;
        return instance;
    }

    uint64_t ran_device::gen_64()
    {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT64_MAX);
        return dist(gen);
    }

    uint32_t ran_device::gen_32()
    {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT32_MAX);
        return dist(gen);
    }

    uint16_t ran_device::gen_16()
    {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT16_MAX);
        return dist(gen);
    }

    uint8_t ran_device::gen_8()
    {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT8_MAX);
        return dist(gen);
    }

    uint64_t ran_device::gen_dist(std::uniform_int_distribution<uint64_t>& distribution)
    {
        return distribution(gen);
    }

    bool ran_device::gen_chance(const float chance)
    {
        std::bernoulli_distribution dist(chance);
        return dist(gen);
    }

    double ran_device::gen_dist(std::uniform_real_distribution<>& distribution)
    {
        return distribution(gen);
    }
}
