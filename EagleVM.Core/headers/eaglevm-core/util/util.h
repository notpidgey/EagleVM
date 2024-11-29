#pragma once
#include <vector>

template <typename T>
std::vector<T>& operator +=(std::vector<T>& A, const std::vector<T>& B)
{
    A.reserve(A.size() + B.size());
    A.insert(A.end(), B.begin(), B.end());
    return A;
}