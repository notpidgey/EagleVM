#pragma once
#include "eaglevm-core/codec/zydis_helper.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_x86_exec : public base_command
    {
    public:
        explicit cmd_x86_exec(const codec::dec::inst_info& dec_req)
            : base_command(command_type::vm_exec_x86)
        {
            request = codec::decode_to_encode(dec_req);
        }

        explicit cmd_x86_exec(const codec::dynamic_instruction& enc_req)
            : base_command(command_type::vm_exec_x86), request(enc_req)
        {
        }

        codec::dynamic_instruction get_request() const
        {
            return request;
        }

    private:
        codec::dynamic_instruction request{ };
    };
}
