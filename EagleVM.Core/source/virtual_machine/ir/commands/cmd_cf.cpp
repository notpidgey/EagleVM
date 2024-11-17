#include "eaglevm-core/virtual_machine/ir/commands/cmd_cf.h"
#include "eaglevm-core/virtual_machine/ir/block.h"
#include <format>

namespace eagle::ir
{
    std::string cmd_call::to_string()
    {
        std::string out;
        out += std::format(" block 0x{:x},", target->block_id);

        out.pop_back();

        return base_command::to_string() + out;
    }

}
