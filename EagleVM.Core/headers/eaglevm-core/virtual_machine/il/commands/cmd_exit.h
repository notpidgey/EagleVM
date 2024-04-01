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
        jump,
        conditional
    };

    class il_bb;
    using il_bb_ptr = std::shared_ptr<il_bb>;

    using vmexit_rva = uint64_t;
    using exit_result = std::variant<vmexit_rva, il_bb_ptr>;

    class cmd_exit : public base_command
    {
    public:
        cmd_exit(const exit_result& result_info, exit_condition exit_condition);
        cmd_exit(const std::vector<exit_result>& result_info, exit_condition exit_condition);

        [[nodiscard]] exit_condition get_condition() const;
        exit_result get_condition_default();
        exit_result get_condition_special();

    private:
        std::array<exit_result, 2> info;
        uint8_t info_size;

        exit_condition condition;
    };
}
