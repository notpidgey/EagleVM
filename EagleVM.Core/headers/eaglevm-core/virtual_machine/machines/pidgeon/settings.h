#pragma once

#define _CREATE_VM_OPTION(type, name, def) private: \
type name = def; \
public: \
type get_##name() { return name; } \
void set_##name (type val) { name = val; }
#include <memory>

namespace eagle::virt::pidg
{
    class settings
    {
    public:
        /*
         * if enabled, the virtual machine will shuffle the register order on VMENTER
         * if disabled, the virtual machine will use the default r0-r15 order
         */
        _CREATE_VM_OPTION(bool, randomize_stack_regs, false)

        /*
         * if enabled, the register manager will shuffle vm registers
         * if disabled, the registers will be left at their original values
         */
        _CREATE_VM_OPTION(bool, randomize_vm_regs, false)

        /*
         *  if enabled, the virtual machine will generate handlers which use several VTEMP(x) registers
         *  if disabled, VTEMP(0) will be used for virtual machine handlers
         */
        _CREATE_VM_OPTION(bool, variant_register_handlers, false)

        /*
         * if enabled, obfuscation utalizing discrete_store_ptr will vary VTEMP(x) registers
         * if disabled, will use the default VTEMP(x) order
         */
        _CREATE_VM_OPTION(bool, randomize_temp_registers, false)

        /*
         * settings "variant_register_handlers" and "randomize_temp_registers" will pull from this pool of randomized registers
         * no less than 2 registers is allowed
         */
        _CREATE_VM_OPTION(uint8_t, temp_count, 2)

        _CREATE_VM_OPTION(bool, random_inline, false)
    };

    using settings_ptr = std::shared_ptr<settings>;
}
