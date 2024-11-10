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
        section_code_containers.push_back(code);
    }

    void section_manager::add_code_container(const std::vector<code_container_ptr>& code)
    {
        section_code_containers.append_range(code);
    }

    codec::encoded_vec section_manager::compile_section(const uint64_t base_address, const uint64_t runtime_base)
    {
        //if (shuffle_functions)
        //    shuffle_containers();

        uint64_t base_offset = base_address;

        // this should take all the functions in the section and connect them to desired labels
        for (const code_container_ptr& code_container : section_code_containers)
        {
            std::vector<codec::encoder::inst_req_label_v> segments = code_container->get_instructions();
            for (auto& label_code_variant : segments)
            {
                std::visit([&base_offset, runtime_base](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, codec::encoder::inst_req>)
                    {
                        const codec::encoder::inst_req inst = arg;
                        codec::enc::req enc = inst.build(base_offset);

                        attempt_instruction_fix(enc);
                        base_offset += codec::compile_absolute(enc, base_offset).size();
                    }
                    else if constexpr (std::is_same_v<T, code_label_ptr>)
                    {
                        const code_label_ptr& label = arg;
                        label->set_address(runtime_base, base_offset);
                    }
                }, label_code_variant);
            }
        }

    RECOMPILE:
        std::vector<uint8_t> compiled_section;
        compiled_section.reserve(base_offset - base_address);

        base_offset = base_address;
        for (const code_container_ptr& code_container : section_code_containers)
        {
            std::vector segments = code_container->get_instructions();
            for (auto& label_code_variant : segments)
            {
                std::vector<uint8_t> compiled;

                bool force_recompile = false;
                std::visit([&base_offset, &force_recompile, runtime_base, &compiled](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, codec::encoder::inst_req>)
                    {
                        const codec::encoder::inst_req inst = arg;
                        codec::enc::req enc = inst.build(base_offset);

                        attempt_instruction_fix(enc);

                        compiled = codec::compile_absolute(enc, 0);
                        base_offset += compiled.size();
                    }
                    else if constexpr (std::is_same_v<T, code_label_ptr>)
                    {
                        const code_label_ptr& label = arg;
                        if (label->get_relative_address() != base_offset)
                            force_recompile = true;

                        label->set_address(runtime_base, base_offset);
                    }
                }, label_code_variant);

                compiled_section.append_range(compiled);

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
        for (const code_container_ptr& code_label : section_code_containers)
        {
            if (code_label->get_is_named())
                json_array.push_back(create_json(code_label));
        }

        return json_array;
    }

    void section_manager::shuffle_containers()
    {
        std::ranges::shuffle(section_code_containers, util::ran_device::get().gen);
    }

    void section_manager::attempt_instruction_fix(codec::enc::req& request)
    {
        if (request.mnemonic == ZYDIS_MNEMONIC_LEA)
        {
            auto& second_op = request.operands[1].mem;
            if (second_op.index == ZYDIS_REGISTER_RSP && second_op.scale == 1)
            {
                auto temp = second_op.base;
                second_op.base = second_op.index;
                second_op.index = temp;
            }
        }
        else if (request.mnemonic == ZYDIS_MNEMONIC_JMP)
        {
            if (request.operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
                request.branch_width = ZYDIS_BRANCH_WIDTH_32;
        }
    }
}
