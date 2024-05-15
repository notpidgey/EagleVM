#include "eaglevm-core/compiler/section_manager.h"
#include "eaglevm-core/util/random.h"

#include <ranges>
#include <variant>

#include "eaglevm-core/codec/zydis_helper.h"

namespace eagle::asmb
{
    section_manager::section_manager()
    {
        shuffle_functions = false;
    }

    section_manager::section_manager(const bool shuffle)
    {
        shuffle_functions = shuffle;
    }

    codec::encoded_vec section_manager::compile_section(const uint64_t section_address)
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
        for (code_container_ptr& code_label : section_labels)
        {
            code_label->set_address(current_address);

            auto segments = code_label->get_instructions();
            for(auto& inst : segments)
            {
                codec::enc::req request;
                std::visit([&request, current_address](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, codec::recompile_promise>)
                        request = arg(current_address);
                    else if constexpr (std::is_same_v<T, codec::enc::req>)
                        request = arg;
                }, inst);

                current_address += codec::compile_absolute(request, 0).size();
            }
        }

    RECOMPILE:

        std::vector<uint8_t> compiled_section;
        compiled_section.reserve(current_address - section_address);

        auto it = compiled_section.begin();

        current_address = section_address;
        for (const code_container_ptr& code_label : section_labels)
        {
            if (code_label->get_address() != current_address)
            {
                code_label->set_address(current_address);
                goto RECOMPILE;
            }

            code_label->set_address(current_address);

            auto segments = code_label->get_instructions();
            for(auto& inst : segments)
            {
                codec::enc::req request;
                std::visit([&request, current_address](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, codec::recompile_promise>)
                        request = arg(current_address);
                    else if constexpr (std::is_same_v<T, codec::enc::req>)
                        request = arg;
                }, inst);

                std::vector<uint8_t> compiled = codec::compile_absolute(request, 0);
                compiled_section.append_range(compiled);
                current_address += compiled.size();
            }
        }

        return compiled_section;
    }

    std::vector<std::string> section_manager::generate_comments(const std::string& output) const
    {
        const std::function create_json = [output](const code_container_ptr& label)
        {
            std::stringstream stream;
            stream << "0x" << std::hex << label->get_name();

            const std::string hex_address(stream.str());

            const std::string manual = "\"manual\": true";
            const std::string module = R"("module": ")" + output + "\"";
            const std::string text = R"("text": ")" + label->get_name() + "\"";
            const std::string address = R"("address": ")" + hex_address + "\"";

            return "{" + manual + "," + module + "," + text + "," + address + "}";
        };

        std::vector<std::string> json_array;
        for (const code_container_ptr& code_label : section_labels)
        {
            if (code_label->get_is_named())
                json_array.push_back(create_json(code_label));
        }

        return json_array;
    }

    void section_manager::perform_shuffle()
    {
        std::ranges::shuffle(section_labels, util::ran_device::get().gen);
    }
}
