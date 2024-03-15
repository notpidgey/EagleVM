#include "eaglevm-core/util/section/function_container.h"
#include "eaglevm-core/util/zydis_helper.h"

#include <iostream>

function_container::function_container()
{
    function_segments.push_back({nullptr, {}});
}

code_label* function_container::assign_label(const std::string& name)
{
    auto* label = new code_label(name);

    function_segments.push_back({label, {}});
    return label;
}

void function_container::assign_label(code_label* label)
{
    function_segments.push_back({label, {}});
}

void function_container::add(const dynamic_instruction& instruction)
{
    auto& [_, instructions] = function_segments.back();
    instructions.push_back(instruction);
}

void function_container::add(std::vector<dynamic_instruction> instruction)
{
    auto& [_, instructions] = function_segments.back();
    instructions.insert(instructions.end(), instruction.begin(), instruction.end());
}

void function_container::add(std::vector<dynamic_instruction>& instruction)
{
    auto& [_, instructions] = function_segments.back();
    instructions.insert(instructions.end(), instruction.begin(), instruction.end());
}

void function_container::add(code_label* target_label, const dynamic_instruction& instruction)
{
    function_segments.push_back({target_label, {instruction}});
}

void function_container::add(code_label* target_label, const std::vector<dynamic_instruction>& instruction)
{
    function_segments.emplace_back(target_label, instruction);
}

function_container& function_container::operator+=(const dynamic_instruction& instruction)
{
    auto& [_, instructions] = function_segments.back();
    instructions.push_back(instruction);
    return *this;
}

function_container& function_container::operator+=(const std::vector<dynamic_instruction>& instruction)
{
    auto& [_, instructions] = function_segments.back();
    instructions.insert(instructions.end(), instruction.begin(), instruction.end());
    return *this;
}

bool function_container::add_to(const code_label* target_label, dynamic_instruction& instruction)
{
    for(auto& [label, instructions] : function_segments)
    {
        if(target_label == label)
        {
            instructions.push_back(instruction);
            return true;
        }
    }

    return false;
}

bool function_container::add_to(const code_label* target_label, std::vector<dynamic_instruction>& instruction)
{
    for(auto& [label, instructions] : function_segments)
    {
        if(target_label == label)
        {
            instructions.insert(instructions.end(), instruction.begin(), instruction.end());
            return true;
        }
    }

    return false;
}

const std::vector<function_segment>& function_container::get_segments() const
{
    return function_segments;
}

size_t function_container::size() const
{
    size_t count = 0;
    for (const auto& segment : function_segments)
    {
        count += segment.second.size();
    }
    return count;
}

void function_container::merge(const function_container& other)
{
    const auto& other_segments = other.get_segments();
    function_segments.insert(function_segments.end(), other_segments.begin(), other_segments.end());
}