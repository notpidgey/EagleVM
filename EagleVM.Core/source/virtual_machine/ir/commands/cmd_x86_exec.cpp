#include "eaglevm-core/virtual_machine/ir/commands/cmd_x86_exec.h"

namespace eagle::ir
{
    cmd_x86_exec::cmd_x86_exec(const codec::encoder::inst_req& enc_req)
        : base_command(command_type::vm_exec_x86), request(enc_req)
    {
    }

    codec::encoder::inst_req cmd_x86_exec::get_request() const
    {
        return request;
    }

    bool cmd_x86_exec::is_similar(const std::shared_ptr<base_command>& other)
    {
        return false;
    }

    std::string cmd_x86_exec::to_string()
    {
        // return std::visit([&](auto&& arg)
        // {
        //     using T = std::decay_t<decltype(arg)>;
        //     if constexpr (std::is_same_v<T, codec::enc::req>)
        //     {
        //         auto reg = arg;
        //         codec::encode
        //         auto str = codec::instruction_to_string(reg);
        //
        //         return base_command::to_string() + " " + str;
        //     }
        //     else
        //     {
        //         return base_command::to_string();
        //     }
        // }, request);

        return base_command::to_string();
    }
}
