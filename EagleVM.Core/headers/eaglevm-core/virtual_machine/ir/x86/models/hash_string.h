#pragma once
#include <cstdint>
#include <string_view>

namespace eagle::ir
{
    namespace hash_string
    {
        static constexpr uint64_t hash(const std::string_view& val)
        {
            constexpr uint64_t hash_offset_base = 14695981039346656037;
            constexpr uint64_t hash_prime = 1099511628211;

            uint64_t hash = hash_offset_base;
            for (const char c : val)
                hash = hash * hash_prime ^ c;

            return hash;
        }
    };
}
