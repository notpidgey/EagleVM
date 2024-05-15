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

        codec::encoded_vec compile_section(uint64_t section_address);
        [[nodiscard]] std::vector<std::string> generate_comments(const std::string& output) const;

        void perform_shuffle();

    private:
        std::vector<code_container_ptr> section_labels;
        bool shuffle_functions = false;
    };
}