#pragma once
#include <Zydis/Zydis.h>
#include <Zycore/LibC.h>

#include <vector>
#include <iostream>
#include <cstddef>

#include "util/zydis_helper.h"

typedef std::vector<ZydisEncoderRequest> handle_instructions;

namespace vm_handle_generator
{
	handle_instructions create_vm_enter(std::vector<short>& push_order);
	handle_instructions create_vm_exit(std::vector<short>& push_order);
}