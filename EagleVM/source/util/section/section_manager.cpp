#include "util/section/section_manager.h"
#include "util/zydis_helper.h"

#include <variant>

encoded_vec section_manager::compile_section(uint32_t section_address)
{
    // i know there are better way to do this without using a variant vector (its disgusting)
    // or recompiling all the instructions 2x
    // but it is the easiest, and this is an open source project
    // if someone would like to, i am open to changes
    // but i am not rewriting this unless i really need to

    uint32_t current_address = section_address;

    // this should take all the functions in the section and connect them to desired labels
    for (auto& [code_label, sec_function] : section_functions)
    {
        uint8_t align = current_address % 16;
        if (align)
            current_address += align;

        if (code_label)
            code_label->finalize(current_address);

        auto& segments = sec_function.get_segments();
        for (auto& [seg_code_label, instructions] : segments)
        {
            if(seg_code_label)
                seg_code_label->finalize(current_address);

            instructions_vec requests;
            for (auto& inst : instructions)
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
            current_address += zydis_helper::encode_queue(requests).size();
        }
    }

    std::vector<uint8_t> compiled_section(current_address - section_address, 0);
    auto it = compiled_section.begin();

    current_address = 0;
    for (auto& [code_label, sec_function] : section_functions)
    {
        uint8_t align = current_address % 16;
        if (align)
        {
            current_address += align;
            std::advance(it, align);
        }

        auto& segments = sec_function.get_segments();
        for (auto& [seg_code_label, instructions] : segments)
        {
            instructions_vec requests;
            for (auto& inst : instructions)
            {
                std::visit([&requests](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, recompile_promise>)
                        requests.push_back(arg());
                    else if constexpr (std::is_same_v<T, zydis_encoder_request>)
                        requests.push_back(arg);
                }, inst);
            }

            std::vector<uint8_t> encoded_instructions = zydis_helper::encode_queue(requests);

            // Calculate the position of the iterator before the insert operation
            size_t pos = std::distance(compiled_section.begin(), it);

            // Insert the elements
            compiled_section.insert(it, encoded_instructions.begin(), encoded_instructions.end());

            // Restore the iterator after the insert operation
            it = compiled_section.begin();
            std::advance(it, pos);

            current_address += encoded_instructions.size();
            std::advance(it, encoded_instructions.size());
        }
    }

    return compiled_section;
}

void section_manager::add(function_container& function)
{
    section_functions.push_back({ nullptr, function });
}

void section_manager::add(code_label* label, function_container& function)
{
    section_functions.push_back({ label, function });
}
