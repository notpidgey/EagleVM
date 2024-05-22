#pragma once
#include <memory>
#include <string>
#include <vector>

#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/compiler/code_label.h"

namespace eagle::asmb
{
    using code_container_ptr = std::shared_ptr<class code_container>;
    using inst_label_v = std::variant<codec::dynamic_instruction, code_label_ptr>;

    class code_container
    {
    public:
        code_container();
        code_container(const std::string& label_name, bool generate_comments);

        static code_container_ptr create();
        static code_container_ptr create(const std::string& label_name, bool generate_comments = true);

        std::string get_name();
        [[nodiscard]] bool get_is_named() const;

        void add(const codec::dynamic_instruction& instruction);
        void add(const std::vector<codec::dynamic_instruction>& instruction);
        void add(std::vector<codec::dynamic_instruction>& instruction);

        void bind_start(const code_label_ptr& code_label);
        void bind(const code_label_ptr& code_label);

        [[nodiscard]] std::vector<inst_label_v> get_instructions() const;

    private:
        std::string name;
        bool is_named;

        std::vector<inst_label_v> function_segments;
    };
}
