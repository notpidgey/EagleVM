#include "util/section/section_manager.h"
#include "util/zydis_helper.h"

void section_manager::finalize_section(uint32_t section_address)
{
    uint32_t current_address = section_address;

    // this should take all the functions in the section and connect them to desired labels
    for (auto& [code_label, sec_function] : section_instructions)
    {
        if (code_label)
            code_label->finalize(current_address);

        std::vector<function_segment>& segments = sec_function.get_segments();
        for (auto& [seg_code_label, instructions] : segments)
        {
            if(seg_code_label)
                seg_code_label->finalize(current_address);

            // zydis does not really have a way of checking the length of an encoded instruction without encoding it
            // so we are going to just encode and check the size... sorry
            std::vector<uint8_t> instructions = zydis_helper::encode_queue(instructions);
            uint32_t segment_size = instructions.size();

            current_address += segment_size;
        }

        uint32_t align = current_address % 16;
        if (align)
            current_address += align;
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
