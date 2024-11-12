#include "eaglevm-core/compiler/section_manager.h"

#include <queue>

#include "eaglevm-core/util/random.h"

#include <ranges>
#include <set>
#include <unordered_set>
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
        if (shuffle_functions)
            shuffle_containers();

        std::vector<std::vector<uint8_t>> output_encodings;
        uint64_t base_offset = base_address;

        std::unordered_map<code_label_ptr, std::set<uint32_t>> label_dependents;

        std::set<uint32_t> rva_dependent_indexes;
        std::unordered_map<code_label_ptr, uint32_t> label_indexes;
        std::vector<codec::encoder::inst_req_label_v> flat_segments;

        for (const code_container_ptr& code_container : section_code_containers)
        {
            for (auto& label_code_variant : code_container->get_instructions())
            {
                std::visit([&](auto&& arg)
                {
                    const uint32_t flat_index = flat_segments.size();

                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, codec::encoder::inst_req>)
                    {
                        const codec::encoder::inst_req inst = arg;

                        // at to label dependency list
                        for (auto& dependent : inst.get_dependents())
                            label_dependents[dependent].insert(flat_index);

                        // add to rva dependency list
                        if (inst.is_rva_dependent())
                            rva_dependent_indexes.insert(flat_index);

                        codec::enc::req enc_req = inst.build(base_offset);
                        attempt_instruction_fix(enc_req);

                        const auto compiled = codec::compile_absolute(enc_req, 0);
                        output_encodings.push_back(compiled);

                        base_offset += compiled.size();

                        flat_segments.push_back(inst);
                    }
                    else if constexpr (std::is_same_v<T, code_label_ptr>)
                    {
                        const code_label_ptr& label = arg;
                        label->set_address(runtime_base, base_offset);

                        // this is stupid but this lets us properly index into output_encodings
                        output_encodings.push_back(std::vector<uint8_t>());

                        VM_ASSERT(!label_indexes.contains(label), "redefined label found");
                        label_indexes[label] = flat_index;

                        flat_segments.push_back(label);
                    }
                }, label_code_variant);
            }
        }

        std::unordered_set<uint32_t> visited_indexes_set;
        std::queue<uint32_t> visit_indexes;
        for (auto& [label, dependent_indexes] : label_dependents)
        {
            // find index of where this label is even placed, this way we can pull all the instructions
            // before it was set, because the instructions after it would have already been compiled.
            // we found the label we were searching for
            uint32_t label_index = label_indexes[label];
            auto it = dependent_indexes.lower_bound(label_index);

            for (auto dep_it = dependent_indexes.begin(); dep_it != it; ++dep_it) {
                visit_indexes.push(*dep_it);
            }
        }

        while (!visit_indexes.empty())
        {
            const uint32_t target_idx = visit_indexes.front();
            visited_indexes_set.erase(target_idx);
            visit_indexes.pop();

            // basically we want to recompile all the instructions in the visit indexes
            // if it changes size, this is a problem because that means all the labels after get redefined
            const size_t original_size = output_encodings[target_idx].size();
            const auto& inst = std::get<codec::encoder::inst_req>(flat_segments[target_idx]);

            uint64_t offset = base_address;
            for (auto i = 0; i < target_idx; i++)
                offset += output_encodings[i].size();

            codec::enc::req enc_req = inst.build(offset);
            attempt_instruction_fix(enc_req);

            const std::vector<uint8_t> compiled = codec::compile_absolute(enc_req, 0);
            output_encodings[target_idx] = compiled;

            if (compiled.size() != original_size)
            {
                // this means that every label after gets redefined
                // we want to collect all the labels defined after this current index
                for (auto& [label, label_idx] : label_indexes)
                {
                    if (label_idx > target_idx)
                    {
                        // this label got redefined
                        // means everything using the label including the label is now invalid
                        for (auto& dependents : label_dependents | std::views::values)
                            for (auto& dependent : dependents)
                                if (std::get<1>(visited_indexes_set.insert(dependent)))
                                    visit_indexes.push(dependent);

                        // we also want to update the label based on the size change
                        const int32_t diff = static_cast<int64_t>(compiled.size()) - static_cast<int64_t>(original_size);
                        label->set_address(runtime_base, label->get_address() + diff);
                    }
                }

                // this also means that every instruction that was depending on the rva, after has changed
                for (auto dep_it = rva_dependent_indexes.upper_bound(target_idx); dep_it != rva_dependent_indexes.end(); ++dep_it) {
                    visit_indexes.push(*dep_it);
                }
            }
        }

        // this code is schizo
        std::vector<uint8_t> flat_vec;
        for (const auto& vec : output_encodings)
            std::ranges::copy(vec, std::back_inserter(flat_vec));

        return flat_vec;
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
