#pragma once
#include <string>

class code_label
{
public:
    explicit code_label(const std::string& section_name);
    static code_label* create(const std::string& section_name);

    uint32_t get();
    void finalize(uint32_t virtual_address);

    std::string get_name();

private:
    std::string name;

    bool finalized;
    uint32_t virtual_address;
};