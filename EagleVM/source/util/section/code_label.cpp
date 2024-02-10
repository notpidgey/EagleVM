#include "util/section/code_label.h"

code_label::code_label(const std::string& section_name)
{
    name = section_name;
    finalized = false;
    virtual_address = 0;
}

code_label* code_label::create(const std::string& section_name)
{
    return new code_label(section_name);
}

uint32_t code_label::get()
{
    return virtual_address;
}

void code_label::finalize(uint32_t va)
{
    finalized = true;
    virtual_address = base_address + va;
}

std::string code_label::get_name()
{
    return name;
}
