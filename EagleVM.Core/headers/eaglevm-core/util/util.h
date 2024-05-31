#pragma once
#include <vector>

template <typename T>
std::vector<T>& operator +=(std::vector<T>& A, const std::vector<T>& B)
{
    A.reserve(A.size() + B.size());
    A.insert(A.end(), B.begin(), B.end());
    return A;
}

namespace test_util
{
    template <typename T, std::same_as<T>... Rest>
    inline std::vector<T> combine_vec(const std::vector<T>& first, const std::vector<Rest>&... rest)
    {
        std::vector<T> result;
        result.reserve((first.size() + ... + rest.size()));
        result.insert(result.end(), first.begin(), first.end());
        (..., result.insert(result.end(), rest.begin(), rest.end()));

        return result;
    }
};
