#include "eaglevm-core/virtual_machine/ir/commands/cmd_x86_exec.h"

namespace eagle::ir
{
    cmd_x86_exec::cmd_x86_exec(const codec::dec::inst_info& dec_req)
        : base_command(command_type::vm_exec_x86)
    {
        request = codec::decode_to_encode(dec_req);
    }

    cmd_x86_exec::cmd_x86_exec(const codec::dynamic_instruction& enc_req)
        : base_command(command_type::vm_exec_x86), request(enc_req)
    {
    }

    codec::dynamic_instruction cmd_x86_exec::get_request() const
    {
        return request;
    }

    bool cmd_x86_exec::is_similar(const std::shared_ptr<base_command>& other)
    {
        return false;
    }
}
