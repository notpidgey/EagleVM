#pragma once
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"
#include "eaglevm-core/codec/zydis_defs.h"

namespace eagle::ir
{
    class cmd_context_store : public base_command
    {
    public:
        explicit cmd_context_store(codec::reg dest);
        explicit cmd_context_store(codec::reg dest, codec::reg_size size);

        codec::reg get_reg() const;
        codec::reg_size get_value_size() const;

        bool is_similar(const std::shared_ptr<base_command>& other) override;
        std::string to_string() override;
        BASE_COMMAND_CLONE(cmd_context_store);

    private:
        codec::reg dest;
        codec::reg_size size;
    };

    SHARED_DEFINE(cmd_context_store);
}
