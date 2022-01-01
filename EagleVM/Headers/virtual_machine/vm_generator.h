#pragma once
#include <map>
#include <vector>
#include <algorithm>
#include <random>
#include <zydis/Zydis.h>

#include "virtual_machine/handle_generator.h"

class vm_generator
{
public:
	void create_vreg_map();
	void create_vm_enter();

private:
	std::vector<char> section_data_;
	std::map<short, short> vreg_mapping_;
};