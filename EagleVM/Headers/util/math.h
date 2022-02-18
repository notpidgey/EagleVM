#pragma once
#include <cstdlib>

namespace math_util
{
	template <typename T>
	inline T create_pran(T min, T max)
	{
		return std::rand() % (max - min + 1) + min;
	}
};