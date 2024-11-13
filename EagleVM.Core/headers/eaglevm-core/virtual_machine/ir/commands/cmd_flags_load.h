#pragma once
#include <array>

#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/codec/zydis_enum.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    enum class vm_flags
    {
        eq = 0, // equals flag
        le = 1, // less than flag
        ge = 2, // greater than flag
    };

    constexpr std::array vm_flags_list = { vm_flags::eq, vm_flags::le, vm_flags::ge };

    class cmd_flags_load final : public base_command
    {
    public:
        explicit cmd_flags_load(vm_flags flag);

        vm_flags get_flag() const;
        bool is_similar(const std::shared_ptr<base_command>& other) override;
        BASE_COMMAND_CLONE(cmd_flags_load);

        static ir_size get_flag_size();
        static uint8_t get_flag_index(vm_flags flag);

    private:
        vm_flags target_load;
    };

    SHARED_DEFINE(cmd_flags_load);
}
