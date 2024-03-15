#include "eaglevm-core/util/section/code_label.h"

code_label::code_label(const std::string& section_name, const bool comment)
{
    name = section_name;
    virtual_address = INT32_MAX;

    finalized = false;
    debug_comment = comment;
}

code_label* code_label::create(const std::string& section_name)
{
    return new code_label(section_name);
}

code_label* code_label::create(const std::string& section_name, const bool generate_comments)
{
    return new code_label(section_name, generate_comments);
}

code_label* code_label::create()
{
    return new code_label("");
}

int64_t code_label::get()
{
    return virtual_address;
}

void code_label::finalize(uint32_t va)
{
    finalized = true;
    virtual_address = va;
}

bool code_label::is_finalized()
{
    return finalized;
}

bool code_label::is_comment()
{
    return debug_comment;
}

std::string code_label::get_name()
{
    return name;
}
