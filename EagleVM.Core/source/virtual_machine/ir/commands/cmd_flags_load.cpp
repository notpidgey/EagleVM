#include "eaglevm-core/virtual_machine/ir/commands/cmd_flags_load.h"
namespace eagle::ir
{
    cmd_flags_load::cmd_flags_load(const vm_flags flag): base_command(command_type::vm_flags_load), target_load(flag)
    { }

    vm_flags cmd_flags_load::get_flag() const
    {
        return target_load;
    }

    bool cmd_flags_load::is_similar(const std::shared_ptr<base_command>& other)
    {
        const auto cmd = std::dynamic_pointer_cast<cmd_flags_load>(other);
        return cmd->get_flag() == get_flag();
    }

    ir_size cmd_flags_load::get_flag_size()
    {
        return ir_size::bit_64;
    }

    uint8_t cmd_flags_load::get_flag_index(const vm_flags flag)
    {
        return static_cast<uint8_t>(flag);
    }
}
