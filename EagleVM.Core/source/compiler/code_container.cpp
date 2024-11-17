#include "eaglevm-core/compiler/code_container.h"

namespace eagle::asmb
{
    uint32_t code_container::current_uid = 0;

    code_container_ptr code_container::create()
    {
        return std::make_shared<code_container>();
    }

    code_container_ptr code_container::create(const std::string& label_name, bool generate_comments)
    {
        return std::make_shared<code_container>(label_name, generate_comments);
    }

    std::string code_container::get_name()
    {
        return name;
    }

    bool code_container::get_is_named() const
    {
        return is_named;
    }

    void code_container::bind_start(const code_label_ptr& code_label)
    {
        instruction_list.insert(instruction_list.begin(), code_label);
    }

    void code_container::add(codec::encoder::inst_req inst)
    {
        instruction_list.push_back(inst);
    }

    std::vector<codec::encoder::inst_req_label_v> code_container::get_instructions() const
    {
        return instruction_list;
    }

    code_container::code_container()
    {
        is_named = false;
        name = "";
        uid = current_uid++;
    }

    code_container::code_container(const std::string& label_name, bool generate_comments)
    {
        is_named = generate_comments;
        name = label_name;
        uid = current_uid++;
    }
}
