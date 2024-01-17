#include "util/section/function_container.h"
#include "util/zydis_helper.h"

#include <iostream>

function_container::function_container()
{
    function_segments.push_back({ nullptr, {} });
}

code_label* function_container::assign_label(const std::string& name)
{
    code_label* label = new code_label(name);

    function_segments.push_back({ label, {} });
    return label;
}

void function_container::add(zydis_encoder_request& instruction)
{
    auto& [_, instructions] = function_segments.back();
    instructions.push_back(instruction);
}

void function_container::add(std::vector<zydis_encoder_request>& instruction)
{
    auto& [_, instructions] = function_segments.back();
    instructions.insert(instructions.end(), instruction.begin(), instruction.end());
}

bool function_container::add(code_label* label, zydis_encoder_request& instruction)
{
    for (auto& [label, instructions] : function_segments)
    {
        if (label == label)
        {
            instructions.push_back(instruction);
            return true;
        }
    }

    return false;
}

bool function_container::add(code_label* label, std::vector<zydis_encoder_request>& instruction)
{
    for (auto& [label, instructions] : function_segments)
    {
        if (label == label)
        {
            instructions.insert(instructions.end(), instruction.begin(), instruction.end());
            return true;
        }
    }

    return false;
}

std::vector<function_segment>& function_container::get_segments()
{
    return function_segments;
}

void function_container::print()
{
    for (auto& [label, instructions] : function_segments)
    {
        std::cout << label->get_name() << std::endl;

        for (zydis_encoder_request& instruction : instructions)
        {
            // std::cout << zydis_helper::print
        }
    }
}
