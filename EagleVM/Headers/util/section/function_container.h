#pragma once
#include <vector>
#include <variant>
#include <functional>

#include "util/section/code_label.h"
#include "util/zydis_defs.h"

typedef std::function<zydis_encoder_request()> recompile_promise;
typedef std::variant<zydis_encoder_request, recompile_promise> dynamic_instruction;
typedef std::pair<code_label*, std::vector<dynamic_instruction>> function_segment;

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

private:
    std::vector<function_segment> function_segments;
};