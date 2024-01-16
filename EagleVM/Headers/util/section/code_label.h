#pragma once
#include <string>

class code_label
{
public:
    explicit code_label(const std::string& section_name);

    std::string get_name();

private:
    std::string name;
};