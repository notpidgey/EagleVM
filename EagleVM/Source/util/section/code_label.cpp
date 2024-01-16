#include "util/section/code_label.h"

code_label::code_label(const std::string& section_name)
{
    name = section_name;
}

std::string code_label::get_name()
{
    return name;
}
