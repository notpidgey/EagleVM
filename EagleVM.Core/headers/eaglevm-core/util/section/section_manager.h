#pragma once
#include "eaglevm-core/util/section/function_container.h"

class code_label;
class section_manager
{
public:
    section_manager();
    explicit section_manager(bool shuffle);

    encoded_vec compile_section(uint64_t section_address);
    std::vector<std::string> generate_comments(const std::string& output);

    void perform_shuffle();

    void add(function_container& function);
    void add(const std::vector<function_container>& functions);
    void add(code_label* label, function_container& function);

    bool valid_label(code_label* label, uint64_t current_address);

private:
    std::vector<std::pair<code_label*, function_container>> section_functions;
    std::vector<std::string> comments;

    bool shuffle_functions = false;
};