#pragma once
#include <string>

class code_label
{
public:
    inline static uint32_t base_address = 0;

    explicit code_label(const std::string& section_name);

    code_label();

    static code_label* create(const std::string& section_name);

    static code_label* create();

    int32_t get();
    void finalize(uint32_t virtual_address);

    std::string get_name();

private:
    std::string name;

    bool finalized;
    uint32_t virtual_address;
};