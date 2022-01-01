#pragma once
#include <map>
#include <vector>
#include <algorithm>
#include <random>
#include <zydis/Zydis.h>

class vm_generator
{
public:
	void create_vreg_map();

private:
	std::map<short, short> vreg_mapping_;
};