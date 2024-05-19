#pragma once

#define _CREATE_VM_OPTION(type, name, def) private: \
type name = def; \
public: \
type get_##name() { return name; } \
void set_##name (type val) { name = val; }

namespace eagle::virt::pidg
{
    class settings
    {
    public:
        _CREATE_VM_OPTION(bool, randomize_stack_regs, false)
        _CREATE_VM_OPTION(bool, variant_register_handlers, false)
        _CREATE_VM_OPTION(bool, randomize_temp_registers, false)
    };
}
