#pragma once
#include <memory>
#include <string>
#include <vector>

#include "eaglevm-core/codec/zydis_defs.h"

namespace eagle::asmb
{
    class code_label;
    using code_label_ptr = std::shared_ptr<code_label>;

    class code_label
    {
    public:
        static code_label_ptr create();
        static code_label_ptr create(const std::string& label_name, bool generate_comments = false);

        std::string get_name();
        [[nodiscard]] bool get_is_comment() const;

        [[nodiscard]] uint64_t get_address() const;
        void set_address(uint64_t address);

        [[nodiscard]] bool get_final() const;
        void set_final(bool final);

        void add(const codec::dynamic_instruction& instruction);
        void add(const std::vector<codec::dynamic_instruction>& instruction);
        void add(std::vector<codec::dynamic_instruction>& instruction);

        [[nodiscard]] std::vector<codec::dynamic_instruction> get_instructions() const;

    private:
        code_label();
        code_label(const std::string& label_name, bool generate_comments);

        std::string name;
        bool comments;

        uint64_t virtual_address;
        bool virtual_address_final;

        std::vector<codec::dynamic_instruction> function_segments;
    };
}
