#pragma once
#include <memory>
#include <string>

namespace eagle::asmb
{
    using code_label_ptr = std::shared_ptr<class code_label>;

    class code_label
    {
    public:
        static code_label_ptr create();
        static code_label_ptr create(const std::string& label_name, bool generate_comments = true);

        std::string get_name();
        [[nodiscard]] bool get_is_named() const;

        [[nodiscard]] uint64_t get_address() const;
        void set_address(uint64_t address);

    private:
        code_label();
        code_label(const std::string& label_name, bool generate_comments);
        
        std::string name;
        bool is_named;
        
        uint64_t virtual_address;
    };
}
