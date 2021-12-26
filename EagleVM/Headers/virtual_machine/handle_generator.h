#pragma once
#include <Zydis/Zydis.h>
#include <Zycore/LibC.h>

#include <vector>
#include <iostream>
#include <cstddef>

#include "util/zydis_helper.h"

struct handle_info
{
	size_t handle_size;
	std::vector<std::vector<uint8_t>> handle_data;
};

namespace vm_handle_generator
{
	handle_info create_vm_enter();
	handle_info create_vm_exit();
}