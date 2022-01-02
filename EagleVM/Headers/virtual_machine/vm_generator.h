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
	void init_vreg_map();
	void init_ran_consts();

	void create_vm_enter();

private:
	std::vector<char> section_data_;
	std::map<short, short> vreg_mapping_;

	union jmp_enc_constants
	{
		//will other for later, this is for test.
		uint8_t values[3] = {};
	} func_address_keys;
};