#pragma once
#include "function_container.h"

class section_manager
{
public:
    void finalize_section(uint32_t section_address);

    void add(function_container& function);
    void add(code_label* label, function_container& function);

private:
    std::vector<std::pair<code_label*, function_container>> section_instructions;
};