#include "eaglevm-core/compiler/code_label.h"

namespace eagle::asmb
{
    code_label_ptr code_label::create()
    {
        return std::make_shared<code_label>();
    }

    code_label_ptr code_label::create(const std::string& label_name, bool generate_comments)
    {
        return std::make_shared<code_label>(label_name, generate_comments);
    }

    std::string code_label::get_name()
    {
        return name;
    }

    bool code_label::get_is_comment() const
    {
        return comments;
    }

    uint64_t code_label::get_address() const
    {
        return virtual_address;
    }

    void code_label::set_address(const uint64_t address)
    {
        virtual_address = address;
    }

    bool code_label::get_final() const
    {
        return virtual_address_final;
    }

    void code_label::set_final(const bool final)
    {
        virtual_address = final;
    }

    void code_label::add(const codec::dynamic_instruction& instruction)
    {
        function_segments.push_back(instruction);
    }

    void code_label::add(const std::vector<codec::dynamic_instruction>& instruction)
    {
        function_segments.append_range(instruction);
    }

    void code_label::add(std::vector<codec::dynamic_instruction>& instruction)
    {
        function_segments.append_range(instruction);
    }

    std::vector<codec::dynamic_instruction> code_label::get_instructions() const
    {
        return function_segments;
    }

    code_label::code_label()
    {
        comments = false;
        name = "";

        virtual_address = 0;
        virtual_address_final = false;
    }

    code_label::code_label(const std::string& label_name, bool generate_comments)
    {
        comments = generate_comments;
        name = label_name;

        virtual_address = 0;
        virtual_address_final = false;
    }
}
