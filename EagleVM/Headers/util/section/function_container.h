#pragma once
#include <vector>

#include "util/section/code_label.h"
#include "util/zydis_defs.h"

typedef std::pair<code_label*, std::vector<zydis_encoder_request>> function_segment;

class function_container
{
public:
    function_container();

    code_label* assign_label(const std::string& name);
    void add(zydis_encoder_request& instruction);
    void add(std::vector<zydis_encoder_request>& instruction);

    bool add(code_label* label, zydis_encoder_request& instruction);
    bool add(code_label* label, std::vector<zydis_encoder_request>& instruction);

    std::vector<function_segment>& get_segments();

    void print();

private:
    std::vector<function_segment> function_segments;
};