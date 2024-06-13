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

    void section_manager::add_code_container(const code_container_ptr& code)
    {
        section_labels.push_back(code);
    }

    void section_manager::add_code_container(const std::vector<code_container_ptr>& code)
    {
        section_labels.append_range(code);
    }

    codec::encoded_vec section_manager::compile_section(const uint64_t section_address)
    {
        if (shuffle_functions)
            shuffle_containers();

        uint64_t current_address = section_address;

        // this should take all the functions in the section and connect them to desired labels
        for (code_container_ptr& code_container : section_labels)
        {
            std::vector<inst_label_v> segments = code_container->get_instructions();
            for (auto& inst : segments)
            {
                std::visit([&current_address](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, codec::dynamic_instruction>)
                    {
                        codec::dynamic_instruction inst = arg;

                        codec::enc::req request;
                        std::visit([&request, current_address](auto&& arg)
                        {
                            using T = std::decay_t<decltype(arg)>;
                            if constexpr (std::is_same_v<T, codec::recompile_promise>)
                                request = arg(current_address);
                            else if constexpr (std::is_same_v<T, codec::enc::req>)
                                request = arg;
                        }, inst);

                        attempt_instruction_fix(request);
                        current_address += codec::compile_absolute(request, 0).size();
                    }
                    else if constexpr (std::is_same_v<T, code_label_ptr>)
                    {
                        const code_label_ptr& label = arg;
                        label->set_address(current_address);
                    }
                }, inst);
            }
        }

    RECOMPILE:

        std::vector<uint8_t> compiled_section;
        compiled_section.reserve(current_address - section_address);

        auto it = compiled_section.begin();

        current_address = section_address;
        for (const code_container_ptr& code_container : section_labels)
        {
            std::vector<inst_label_v> segments = code_container->get_instructions();
            for (inst_label_v& label_code_variant : segments)
            {
                bool force_recompile = false;
                std::visit([&current_address, &compiled_section, &force_recompile](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, codec::dynamic_instruction>)
                    {
                        codec::dynamic_instruction dynamic_inst = arg;

                        codec::enc::req request;
                        std::visit([&request, current_address](auto&& arg)
                        {
                            using T = std::decay_t<decltype(arg)>;
                            if constexpr (std::is_same_v<T, codec::recompile_promise>)
                                request = arg(current_address);
                            else if constexpr (std::is_same_v<T, codec::enc::req>)
                                request = arg;
                        }, dynamic_inst);

                        attempt_instruction_fix(request);

                        std::vector<uint8_t> compiled = codec::compile_absolute(request, 0);
                        compiled_section.append_range(compiled);

                        current_address += compiled.size();
                    }
                    else if constexpr (std::is_same_v<T, code_label_ptr>)
                    {
                        const code_label_ptr& label = arg;
                        if (label->get_address() != current_address)
                            force_recompile = true;

                        label->set_address(current_address);
                    }
                }, label_code_variant);

                if (force_recompile)
                    goto RECOMPILE;
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

    void section_manager::shuffle_containers()
    {
        std::ranges::shuffle(section_labels, util::ran_device::get().gen);
    }

    void section_manager::attempt_instruction_fix(codec::enc::req& request)
    {
        if(request.mnemonic == ZYDIS_MNEMONIC_LEA)
        {
            auto& second_op = request.operands[1].mem;
            if(second_op.index == ZYDIS_REGISTER_RSP && second_op.scale == 1)
            {
                auto temp = second_op.base;
                second_op.base = second_op.index;
                second_op.index = temp;
            }
        }
    }
}
