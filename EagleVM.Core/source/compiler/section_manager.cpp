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

    encoded_vec section_manager::compile_section(const uint64_t base_address, const uint64_t runtime_base)
    {
        if (shuffle_functions)
            shuffle_containers();

        uint64_t base_offset = base_address;

        std::unordered_map<code_label_ptr, uint32_t> label_begin_use;
        std::unordered_map<code_label_ptr, std::vector<uint32_t>> label_dependents;
        std::vector<inst_label_v> flat_requests;

        // flatten the instruction containers
        for (const code_container_ptr& code_container : section_code_containers)
        {
            std::vector<inst_label_v> segments = code_container->get_instructions();
            flat_requests.append_range(segments);
        }

        // pre allocate memory reserved for instruction
        // maximum length can only be 15 bytes, however we may get large sets of instructions which will allocate more memory
        std::unordered_map<uint32_t, std::vector<uint8_t>> compiled_instructions(flat_requests.size());
        for (auto i = 0; i < flat_requests.size(); i++)
        {
            auto& vec = compiled_instructions[i];
            vec.reserve(15);
        }

        // here we must do two things to optimize future compilation
        // initialize label occurances
        // compile static instructions which in most cases take up majority of the compilation
        for (auto i = 0; i < flat_requests.size(); i++)
        {
            auto label_code_variant = flat_requests[i];
            std::visit([&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, code_label_ptr>)
                {
                    const code_label_ptr& label = arg;
                    if (!label_begin_use.contains(label))
                        label_begin_use[label] = i;
                }
                else if constexpr (std::is_same_v<T, dynamic_instruction>)
                {
                    dynamic_instruction dynamic_inst = arg;
                    std::visit([&](auto&& req_arg)
                    {
                        using T = std::decay_t<decltype(req_arg)>;
                        if constexpr (!std::is_same_v<T, recompile_chunk>)
                        {
                            codec::enc::req request;
                            if constexpr (std::is_same_v<T, codec::enc::req>)
                                request = req_arg;

                            attempt_instruction_fix(request);

                            if (!codec::has_relative_operand(request))
                            {
                                const std::vector<uint8_t> compiled = codec::compile_absolute(request, 0);
                                compiled_instructions[i].assign(compiled.begin(), compiled.end());
                            }
                        }
                    }, dynamic_inst);
                }
            }, label_code_variant);
        }

        std::unordered_map<uint32_t, uint32_t> request_sizes;

        base_offset = base_address;
        for (uint32_t i = 0; i < flat_requests.size(); i++)
        {
            auto& label_code_variant = flat_requests[i];

            code_label_ptr relocated_label = nullptr;
            std::visit([&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, dynamic_instruction>)
                {
                    dynamic_instruction dynamic_inst = arg;
                    std::visit([&](auto&& arg)
                    {
                        std::vector<uint8_t> compiled;

                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, recompile_chunk>)
                        {
                            compiled = arg(base_offset);
                            base_offset += compiled.size();
                        }
                        else
                        {
                            codec::enc::req request;
                            if constexpr (std::is_same_v<T, recompile_promise>)
                                request = arg(base_offset);
                            else if constexpr (std::is_same_v<T, codec::enc::req>)
                                request = arg;
                            else
                                __debugbreak();

                            attempt_instruction_fix(request);

                            compiled = codec::compile_absolute(request, 0);
                            base_offset += compiled.size();
                        }


                    }, dynamic_inst);
                }
                else if constexpr (std::is_same_v<T, code_label_ptr>)
                {
                    const code_label_ptr& label = arg;
                    if (label->get_relative_address() != base_offset)
                        relocated_label = label;

                    label->set_address(runtime_base, base_offset);
                }
            }, label_code_variant);

            if (relocated_label)
            {
                // this means that whatever instructions where before it, change in size
                // which caused this label to shift in position
                // we want to check where the first occurance of this label is.

                uint32_t first_occurance = label_begin_use[relocated_label];
                if (first_occurance < i)
                {
                    // this means that the first occurance of this was before this label was declared,
                    // this means we have to go back and recompile all prior instructions

                    i = first_occurance;
                }

                /*
                 * code <- label 2
                 * label 1
                 * code <- label 1
                 * label 2
                 * code <- label 1
                 * code <- label 2
                 */
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
