#include "eaglevm-core/util/section/section_manager.h"

#include "eaglevm-core/util/zydis_helper.h"
#include "eaglevm-core/util/random.h"

#include <ranges>
#include <variant>

section_manager::section_manager()
{
    shuffle_functions = false;
}

section_manager::section_manager(bool shuffle)
{
    shuffle_functions = shuffle;
}

encoded_vec section_manager::compile_section(const uint32_t section_address)
{
    // i know there are better way to do this without using a variant vector (its disgusting)
    // or recompiling all the instructions 2x
    // but it is the easiest, and this is an open source project
    // if someone would like to, i am open to changes
    // but i am not rewriting this unless i really need to

    if (shuffle_functions)
        perform_shuffle();

    uint32_t current_address = section_address;

    // this should take all the functions in the section and connect them to desired labels
    for (auto& [code_label, sec_function]: section_functions)
    {
        const uint8_t align = current_address % 16 == 0 ? 0 : 16 - (current_address % 16);
        current_address += align;

        if (code_label)
            code_label->finalize(current_address);

        auto& segments = sec_function.get_segments();
        for (auto& [seg_code_label, instructions]: segments)
        {
            if (seg_code_label)
                seg_code_label->finalize(current_address);

            for (auto& inst: instructions)
            {
                zydis_encoder_request request;
                std::visit([&request, current_address](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, recompile_promise>)
                        request = arg(current_address);
                    else if constexpr (std::is_same_v<T, zydis_encoder_request>)
                        request = arg;
                }, inst);

                current_address += zydis_helper::compile_absolute(request, 0).size();
            }
        }
    }

    std::vector<uint8_t> compiled_section(current_address - section_address, 0);
    auto it = compiled_section.begin();

    current_address = section_address;
    for (auto& [code_label, sec_function]: section_functions)
    {
        const uint8_t align = current_address % 16 == 0 ? 0 : 16 - (current_address % 16);
        current_address += align;

        if (code_label && !valid_label(code_label, current_address))
            code_label->finalize(current_address);

        std::advance(it, align);

        auto& segments = sec_function.get_segments();
        for (auto& [seg_code_label, instructions]: segments)
        {
            if (seg_code_label && !seg_code_label->is_finalized())
                __debugbreak();

            if (seg_code_label && !valid_label(seg_code_label, current_address))
                seg_code_label->finalize(current_address);

            std::vector<uint8_t> compiled_instructions;
            for (auto& inst: instructions)
            {
                zydis_encoder_request request;
                std::visit([&request, current_address](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, recompile_promise>)
                        request = arg(current_address);
                    else if constexpr (std::is_same_v<T, zydis_encoder_request>)
                        request = arg;
                }, inst);

                std::vector<uint8_t> compiled = zydis_helper::compile_absolute(request, 0);
                compiled_instructions.append_range(compiled);
                current_address += compiled.size();
            }

            // Calculate the position of the iterator before the insert operation
            size_t pos = std::distance(compiled_section.begin(), it);

            // Insert the elements
            compiled_section.insert(it, compiled_instructions.begin(), compiled_instructions.end());

            // Restore the iterator after the insert operation
            it = compiled_section.begin();
            std::advance(it, pos);

            std::advance(it, compiled_instructions.size());
        }
    }

    return compiled_section;
}

std::vector<std::string> section_manager::generate_comments(const std::string& output)
{
    const std::function create_json = [output](code_label* label)
    {
        std::stringstream stream;
        stream << "0x" << std::hex << label->get();
        std::string hex_address(stream.str());

        std::string manual = "\"manual\": true";
        std::string module = "\"module\": \"" + output + "\"";
        std::string text = "\"text\": \"" + label->get_name() + "\"";
        std::string address = "\"address\": \"" + hex_address + "\"";

        return "{" + manual + "," + module + "," + text + "," + address + "}";
    };

    std::vector<std::string> json_array;
    for (auto& [code_label, sec_function]: section_functions)
    {
        if (code_label && code_label->is_comment())
            json_array.push_back(create_json(code_label));

        auto& segments = sec_function.get_segments();
        for (const auto& seg_code_label: segments | std::views::keys)
            if (seg_code_label && seg_code_label->is_comment())
                json_array.push_back(create_json(seg_code_label));
    }

    return json_array;
}

void section_manager::add(function_container& function)
{
    section_functions.push_back({nullptr, function});
}

void section_manager::add(const std::vector<function_container>& functions)
{
    for (int i = 0; i < functions.size(); i++)
        section_functions.push_back({nullptr, functions[i]});
}

void section_manager::add(code_label* label, function_container& function)
{
    section_functions.push_back({label, function});
}

bool section_manager::valid_label(code_label* label, uint32_t current_address)
{
    auto label_address = label->get();
    if (label_address != current_address)
        return false;

    return true;
}

void section_manager::perform_shuffle()
{
    std::ranges::shuffle(section_functions, ran_device::get().gen);
}
