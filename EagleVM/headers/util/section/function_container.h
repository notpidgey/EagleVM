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
    void assign_label(code_label* label);

    void add(const dynamic_instruction& instruction);
    void add(std::vector<dynamic_instruction> instruction);
    void add(std::vector<dynamic_instruction>& instruction);

    void add(code_label* target_label, const dynamic_instruction& instruction);
    void add(code_label* target_label, const std::vector<dynamic_instruction>& instruction);

    function_container& operator+=(const dynamic_instruction& instruction);
    function_container& operator+=(const std::vector<dynamic_instruction>& instruction);

    bool add_to(const code_label* target_label, dynamic_instruction& instruction);
    bool add_to(const code_label* target_label, std::vector<dynamic_instruction>& instruction);

    std::vector<function_segment>& get_segments();
    size_t size() const;

private:
    std::vector<function_segment> function_segments;
};