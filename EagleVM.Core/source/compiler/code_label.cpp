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

    bool code_label::get_is_named() const
    {
        return is_named;
    }

    uint64_t code_label::get_address() const
    {
        return virtual_address;
    }

    void code_label::set_address(const uint64_t address)
    {
        virtual_address = address;
    }

    code_label::code_label()
    {
        is_named = false;
        name = "";

        virtual_address = 0;
    }

    code_label::code_label(const std::string& label_name, const bool generate_comments)
    {
        is_named = generate_comments;
        name = label_name;

        virtual_address = 0;
    }
}