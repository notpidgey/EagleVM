#pragma once
#include "util/zydis_defs.h"
#include "util/section/code_label.h"

struct virtual_basic_block
{
    code_label* start_rva;
    dynamic_instructions_vec instructions;
};
