#pragma once
#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_x86_exec : public base_command
    {
    public:
        explicit cmd_x86_exec(const codec::dec::inst_info& dec_req);
        explicit cmd_x86_exec(const codec::dynamic_instruction& enc_req);

        codec::dynamic_instruction get_request() const;

        bool is_similar(const std::shared_ptr<base_command>& other) override;

        std::string to_string() override;

    private:
        codec::dynamic_instruction request{ };
    };

    SHARED_DEFINE(cmd_x86_exec);
}
