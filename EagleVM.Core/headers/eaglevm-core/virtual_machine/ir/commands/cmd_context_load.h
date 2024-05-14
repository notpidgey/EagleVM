#pragma once
#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/codec/zydis_enum.h"
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    class cmd_context_load : public base_command
    {
    public:
        explicit cmd_context_load(codec::reg source);

        codec::reg get_reg() const;
        codec::reg_class get_reg_class() const;

    private:
        codec::reg source = codec::reg::none;
        codec::reg_class r_class = codec::reg_class::invalid;
    };
}
