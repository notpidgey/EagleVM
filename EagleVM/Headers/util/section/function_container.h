#pragma once
#include <vector>

#include "util/section/code_label.h"
#include "util/zydis_defs.h"

class function_container
{
public:
    function_container();

    code_label* assign_label(const std::string& name);
    void add(zydis_encoder_request& instruction);
    bool add(code_label* label, zydis_encoder_request& instruction);

    void print();

private:
    std::vector<std::pair<code_label*, std::vector<zydis_encoder_request>>> function_segments;
};