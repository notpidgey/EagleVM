#include "util/section/code_label.h"

code_label::code_label(const std::string& section_name)
{
    name = section_name;
    finalized = false;
    virtual_address = INT32_MAX;
}

code_label::code_label()
{
    finalized = false;
    virtual_address = INT32_MAX;
}

code_label* code_label::create(const std::string& section_name)
{
    return new code_label(section_name);
}

code_label* code_label::create()
{
    return new code_label();
}

int32_t code_label::get()
{
    return virtual_address;
}

void code_label::finalize(uint32_t va)
{
    finalized = true;
    virtual_address = base_address + va;
}

bool code_label::is_finalized()
{
    return finalized;
}

std::string code_label::get_name()
{
    return name;
}
