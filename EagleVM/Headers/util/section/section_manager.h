#pragma once
#include "function_container.h"

class section_manager
{
public:
    void bake_section(uint32_t section_address);

private:
    std::vector<> section_instructions;
};