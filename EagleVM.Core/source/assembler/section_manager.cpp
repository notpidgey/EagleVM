#include "eaglevm-core/assembler/section_manager.h"

#include "eaglevm-core/util/zydis_helper.h"
#include "eaglevm-core/util/random.h"

#include <ranges>
#include <variant>

namespace eagle::asmbl
{
    section_manager::section_manager()
    {
        shuffle_functions = false;
    }

    section_manager::section_manager(bool shuffle)
    {
        shuffle_functions = shuffle;
    }

    encoded_vec section_manager::compile_section(const uint64_t section_address)
    {
        assert(section_address % 16 == 0 && "Section address must be aligned to 16 bytes");
        // i know there are better way to do this without using a variant vector (its disgusting)
        // or recompiling all the instructions 2x
        // but it is the easiest, and this is an open source project
        // if someone would like to, i am open to changes
        // but i am not rewriting this unless i really need to

        if (shuffle_functions)
            perform_shuffle();

        uint64_t current_address = section_address;

        // this should take all the functions in the section and connect them to desired labels
        for (auto& [code_label, sec_function] : section_functions)
        {
            if (code_label)
                code_label->finalize(current_address);

            auto& segments = sec_function.get_segments();
            for (auto& [seg_code_label, instructions] : segments)
            {
                if (seg_code_label)
                    seg_code_label->finalize(current_address);

                for (auto& inst : instructions)
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

    RECOMPILE:

        std::vector<uint8_t> compiled_section;
        compiled_section.reserve(current_address - section_address);

        auto it = compiled_section.begin();

        current_address = section_address;
        for (auto& [code_label, sec_function] : section_functions)
        {
            if (code_label)
            {
                if (code_label->get() != current_address)
                {
                    code_label->finalize(current_address);
                    goto RECOMPILE;
                }

                code_label->finalize(current_address);
            }

            auto& segments = sec_function.get_segments();
            for (auto& [seg_code_label, instructions] : segments)
            {
                if (seg_code_label)
                {
                    if (seg_code_label->get() != current_address)
                    {
                        seg_code_label->finalize(current_address);
                        goto RECOMPILE;
                    }

                    seg_code_label->finalize(current_address);
                }

                for (auto& inst : instructions)
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
                    compiled_section.append_range(compiled);
                    current_address += compiled.size();
                }
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
        for (auto& [code_label, sec_function] : section_functions)
        {
            if (code_label && code_label->is_comment())
                json_array.push_back(create_json(code_label));

            auto& segments = sec_function.get_segments();
            for (const auto& seg_code_label : segments | std::views::keys)
                if (seg_code_label && seg_code_label->is_comment())
                    json_array.push_back(create_json(seg_code_label));
        }

        return json_array;
    }

    void section_manager::add(function_container& function)
    {
        section_functions.emplace_back(nullptr, function);
    }

    void section_manager::add(const std::vector<function_container>& functions)
    {
        for (const auto& function : functions)
            section_functions.emplace_back(nullptr, function);
    }

    void section_manager::add(code_label* label, function_container& function)
    {
        section_functions.emplace_back(label, function);
    }

    bool section_manager::valid_label(code_label* label, uint64_t current_address)
    {
        auto label_address = label->get();
        if (label_address != current_address)
            return false;

        return true;
    }

    void section_manager::perform_shuffle()
    {
        std::ranges::shuffle(section_functions, util::ran_device::get().gen);
    }
}
