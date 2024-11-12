#pragma once
#include "eaglevm-core/compiler/code_container.h"

namespace eagle::asmb
{
    class code_container;
    class section_manager
    {
    public:
        section_manager();
        explicit section_manager(bool shuffle);

        void add_code_container(const code_container_ptr& code);
        void add_code_container(const std::vector<code_container_ptr>& code);

        codec::encoded_vec compile_section(uint32_t base);
        [[nodiscard]] std::vector<std::string> generate_comments(const std::string& output) const;

        void shuffle_containers();

    private:
        std::vector<code_container_ptr> section_code_containers;
        bool shuffle_functions = false;

        static void attempt_instruction_fix(codec::enc::req& request);
    };
}