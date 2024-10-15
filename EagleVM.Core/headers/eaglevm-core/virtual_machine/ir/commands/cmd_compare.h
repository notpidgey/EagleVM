#pragma once
#include <array>
#include <vector>
#include "eaglevm-core/virtual_machine/ir/commands/base_command.h"

namespace eagle::ir
{
    template <command_type T>
    class cmd_compare : public base_command
    {
    public:
        cmd_base(ir_size size) : base_command(T), size(size) {}

        ir_size get_size() const { return size; }

    private:
        ir_size size;
    };

    /*
        the cmd_test IR instruction compares the second top most element to the top most element
        the result of the instruction is a ir_size::bit_64 flag which is then pushed to the stack
        
    */
    using cmd_test = cmd_compare<command_type::vm_test>;
    using cmd_cmp = cmd_compare<command_type::vm_cmp>;
}
