#pragma once
#include "eaglevm-core/virtual_machine/il/commands/base_command.h"

namespace eagle::il
{
    class vm_param : public base_command
    {
    public:
        explicit vm_param(const uint8_t param_index, const reg_size param_size, vm_handler_call_ptr call_ptr)
            : base_command(command_type::vm_param), param_index(param_index), param_size(param_size), call_ptr(std::move(call_ptr))
        {

        }

        uint8_t get_param_index();
        reg_size get_param_size();
        vm_handler_call get_call_ptr();

    private:
        uint8_t param_index;
        reg_size param_size;

        vm_handler_call_ptr call_ptr;
    };
}
