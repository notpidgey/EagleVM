#include "eaglevm-core/virtual_machine/il/x86/base_handler_gen.h"

namespace eagle::il
{
    bool handler::base_handler_gen::is_instruction_supported(codec::reg_class operand_width, uint8_t operands)
    {
        // TODO: this needs a future rework where every handler has an operand type and operand width
        // this should not be as simple as checking the instruction width because m
        return std::ranges::any_of(entries,
            [=](auto& handler)
            {
                return handler.operand_width == operand_width && operands == handler.operand_count;
            });
    }
}
