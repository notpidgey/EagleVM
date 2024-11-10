#pragma once
#include <memory>
#include <string>
#include <vector>

#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/codec/zydis_encoder.h"
#include "eaglevm-core/compiler/code_label.h"

namespace eagle::asmb
{
    using code_container_ptr = std::shared_ptr<class code_container>;
    class code_container : public codec::encoder::encode_builder
    {
    public:
        code_container();
        code_container(const std::string& label_name, bool generate_comments);

        static code_container_ptr create();
        static code_container_ptr create(const std::string& label_name, bool generate_comments = true);

        std::string get_name();
        [[nodiscard]] bool get_is_named() const;

        void bind_start(const code_label_ptr& code_label);
        void add(codec::encoder::inst_req inst);

        [[nodiscard]] std::vector<codec::encoder::inst_req_label_v> get_instructions() const;

    private:
        uint32_t uid;
        static uint32_t current_uid;

        std::string name;
        bool is_named;
    };
}
