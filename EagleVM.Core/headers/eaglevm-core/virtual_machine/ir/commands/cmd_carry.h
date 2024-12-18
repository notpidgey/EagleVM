#pragma once
#include <utility>

#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_carry : public base_command
    {
    public:
        explicit cmd_carry(ir_size value_size, uint16_t byte_move_size);

        ir_size get_size() const;
        uint16_t get_move_size() const;

        bool is_similar(const std::shared_ptr<base_command>& other) override;
        std::string to_string() override;
        BASE_COMMAND_CLONE(cmd_carry);

    private:
        ir_size size;
        uint16_t byte_move_size;
    };

    SHARED_DEFINE(cmd_carry);
}
