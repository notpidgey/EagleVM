#pragma once
#include <map>
#include <vector>
#include <algorithm>
#include <random>
#include <intrin.h>

#include <zydis/Zydis.h>

#include "virtual_machine/handle_generator.h"

class vm_generator
{
public:
	vm_generator();

	void init_vreg_map();
	void init_ran_consts();

	std::vector<uint8_t> create_padding(size_t bytes);

	std::vector<uint8_t> create_vm_enter_jump(uint32_t va_protected);
	std::vector<uint8_t> create_vm_enter();

private:
	uint32_t vm_enter_va = 0x1;
	uint32_t vm_exit_va = 0x1;

	std::vector<char> section_data_;
	std::vector<short> reg_order_;

	union jmp_enc_constants
	{
		//will other for later, this is for test.
		uint8_t values[3] = {};
	} func_address_keys;
};