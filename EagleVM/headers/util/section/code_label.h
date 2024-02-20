#pragma once
#include <string>

class code_label
{
public:
    explicit code_label(const std::string& section_name, bool comment = false);

    static code_label* create(const std::string& section_name);
    static code_label* create(const std::string& section_name, bool generate_comments);
    static code_label* create();

    int32_t get();
    void finalize(uint32_t virtual_address);

    bool is_finalized();
    bool is_comment();

    std::string get_name();

private:
    std::string name;
    uint32_t virtual_address;

    bool finalized;
    bool debug_comment;
};