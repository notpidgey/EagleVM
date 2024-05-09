#pragma once
#include <array>
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

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

    class block_il;
    using block_il_ptr = std::shared_ptr<block_il>;

    using vmexit_rva = uint64_t;
    using il_exit_result = std::variant<vmexit_rva, block_il_ptr>;

    class cmd_exit : public base_command
    {
    public:
        cmd_exit(const il_exit_result& result_info, exit_condition exit_condition);
        cmd_exit(const std::vector<il_exit_result>& result_info, exit_condition exit_condition);

        [[nodiscard]] exit_condition get_condition() const;
        il_exit_result get_condition_default();
        il_exit_result get_condition_special();

    private:
        std::array<il_exit_result, 2> info;
        uint8_t info_size;

        exit_condition condition;
    };
}
