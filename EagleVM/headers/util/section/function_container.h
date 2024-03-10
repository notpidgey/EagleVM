#pragma once
#include <vector>
#include <variant>
#include <functional>

#include "util/section/code_label.h"
#include "util/zydis_defs.h"

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

    const std::vector<function_segment>& get_segments() const;
    size_t size() const;

    void merge(const function_container& other);

private:
    std::vector<function_segment> function_segments;
};

/*
 * brainstorming:
 * potential look at what container add might do
 *
 * code_label* label = code_label::create();
 * container.add([=](dynamic_instruction_vec& vec){
 *      vec += enc(ZYDIS_MNEMONIC_PUSHFQ),
 *      vec += enc(code_label, ZYDIS_MNEMONIC_PUSHFQ)
 *      vec += enc(ZYDIS_MNEMONIC_MOV, ZREG(GR_R11), ZIMMU(label->get()))
 * })
 */