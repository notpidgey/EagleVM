#pragma once
#include <random>

class ran_device
{
public:
    std::random_device rd;
    std::mt19937 gen;

    ran_device()
    {
        #if _DEBUG
        gen.seed(2454160668);
        #else
        gen.seed(2454160668);
        #endif
    }

    static ran_device& get()
    {
        static ran_device instance;
        return instance;
    }

    uint64_t gen_64()
    {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT64_MAX);
        return dist(gen);
    }

    uint32_t gen_32()
    {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT32_MAX);
        return dist(gen);
    }

    uint16_t gen_16()
    {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT16_MAX);
        return dist(gen);
    }

    uint8_t gen_8()
    {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT8_MAX);
        return dist(gen);
    }

    uint64_t gen_dist(std::uniform_int_distribution<uint64_t>& distribution)
    {
        std::random_device rd;
        std::mt19937 gen(rd());

        return distribution(gen);
    }

    ran_device(const ran_device&) = delete;
    ran_device& operator=(const ran_device&) = delete;
};