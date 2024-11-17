#pragma once
#include "eaglevm-core/codec/zydis_encoder.h"
#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_x86_exec : public base_command
    {
    public:
        explicit cmd_x86_exec(const codec::encoder::inst_req& enc_req);

        codec::encoder::inst_req get_request() const;

        bool is_similar(const std::shared_ptr<base_command>& other) override;
        std::string to_string() override;
        BASE_COMMAND_CLONE(cmd_x86_exec);

    private:
        codec::encoder::inst_req request;
    };

    SHARED_DEFINE(cmd_x86_exec);
}
