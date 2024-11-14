#pragma once
#include <utility>

#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_pop : public base_command
    {
    public:
        explicit cmd_pop(ir_size size);

        ir_size get_size() const;

        bool is_similar(const std::shared_ptr<base_command>& other) override;
        std::string to_string() override;
        BASE_COMMAND_CLONE(cmd_pop);

    private:
        ir_size size;
    };

    SHARED_DEFINE(cmd_pop);
}
