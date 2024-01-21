#include "util/section/section_manager.h"
#include "util/zydis_helper.h"

#include <variant>

void section_manager::finalize_section(uint32_t section_address)
{
    uint32_t current_address = section_address;

    // this should take all the functions in the section and connect them to desired labels
    for (auto& [code_label, sec_function] : section_instructions)
    {
        uint8_t align = current_address % 16;
        if (align)
            current_address += align;

        if (code_label)
            code_label->finalize(current_address);

        std::vector<function_segment>& segments = sec_function.get_segments();
        for (auto& [seg_code_label, instructions] : segments)
        {
            if(seg_code_label)
                seg_code_label->finalize(current_address);

            instructions_vec requests;
            for (dynamic_instruction& inst : instructions)
            {
                std::visit([&requests](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, recompile_promise>)
                        requests.push_back(arg());
                    else if constexpr (std::is_same_v<T, zydis_encoder_request>)
                        requests.push_back(arg);
                }, inst);
            }

            // zydis does not really have a way of checking the length of an encoded instruction without encoding it
            // so we are going to just encode and check the size... sorry
            std::vector<uint8_t> instructions = zydis_helper::encode_queue(requests);
            uint32_t segment_size = instructions.size();

            current_address += segment_size;
        }
    }

    std::vector<uint8_t> compiled_section(current_address, 0);
    auto it = compiled_section.begin();

    current_address = 0;
    for (auto& [code_label, sec_function] : section_instructions)
    {
        uint8_t align = current_address % 16;
        if (align)
        {
            current_address += align;
            std::advance(it, align);
        }

        std::vector<function_segment>& segments = sec_function.get_segments();
        for (auto& [seg_code_label, instructions] : segments)
        {
            instructions_vec requests;
            for (dynamic_instruction& inst : instructions)
            {
                std::visit([&requests](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, recompile_promise>)
                        requests.push_back(arg());
                    else if constexpr (std::is_same_v<T, zydis_encoder_request>)
                        requests.push_back(arg);
                }, inst);
            }

            // zydis does not really have a way of checking the length of an encoded instruction without encoding it
            // so we are going to just encode and check the size... sorry
            std::vector<uint8_t> instructions = zydis_helper::encode_queue(requests);
            compiled_section.insert(it, instructions.begin(), instructions.end());

            current_address += instructions.size();
            std::advance(it, instructions.size());
        }
    }
}

void section_manager::add(function_container& function)
{
    section_instructions.push_back({ nullptr, function });
}

void section_manager::add(code_label* label, function_container& function)
{
    section_instructions.push_back({ label, function });
}
