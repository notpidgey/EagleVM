#pragma once
#include "eaglevm-core/codec/zydis_defs.h"
#include "eaglevm-core/codec/zydis_enum.h"
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"

namespace eagle::il
{
    class cmd_context_load : public base_command
    {
    public:
        explicit cmd_context_load(const codec::reg source, const il_size size)
            : base_command(command_type::vm_context_load),
              source(source), size(size)
        {
        }

        explicit cmd_context_load(const codec::reg source, const codec::reg_size size)
            : base_command(command_type::vm_context_load),
              source(source), size(static_cast<il_size>(size))
        {
        }

    private:
        codec::reg source = codec::reg::none;
        il_size size = il_size::bit_64;
    };
}