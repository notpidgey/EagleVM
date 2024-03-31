#pragma once
#include <array>
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"

namespace eagle::il
{
    enum class exit_location
    {
        none,
        block,
        rva
    };

    enum class exit_condition
    {
        none,
        exit,
        conditional
    };

    class il_bb;
    using il_bb_ptr = std::shared_ptr<il_bb>;

    using vmexit_rva = uint64_t;
    using exit_result_v = std::variant<vmexit_rva, il_bb>;
    using exit_result_info = std::pair<exit_result_v, exit_condition>;

    class vm_exit : public base_command
    {
    public:
        vm_exit(exit_result_info result_info, exit_condition exit_condition);
        vm_exit(const std::vector<exit_result_info>& result_info, exit_condition exit_condition);

        exit_condition get_condition();
        exit_result_v get_condition_default();
        exit_result_v get_condition_special();

    private:
        std::array<exit_result_info, 2> info;
        exit_condition condition;
    };
}
