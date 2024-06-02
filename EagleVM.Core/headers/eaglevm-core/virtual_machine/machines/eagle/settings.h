#pragma once
#include <memory>

namespace eagle::virt::eg
{
    using settings_ptr = std::shared_ptr<struct settings>;

    struct settings
    {
        /**
         * when enabled, handlers will only be generated a single time
         * when disabled, handler will be generated every time time per call
         */
        bool single_use_x86_handlers = false;

        /**
         * when "single_use_x86_handlers" is set to false,
         * this value will be used to determine the chance of generating a new handler
         *
         * recommended value: 1.0
         */
        float chance_to_generate_x86_handler = 1.0;

        /**
         * when enabled, handlers will only be generated a single time
         * when disabled, handler will be generated every time time per call
         */
        bool single_use_vm_handlers = false;

        /**
         * when "single_use_vm_handlers" is set to false,
         * this value will be used to determine the chance of generating a new handler
         *
         * recommended value: 1.0
         */
        float chance_to_generate_vm_handler = 1.0;

        /**
        * when enabled, handlers will only be generated a single time
        * when disabled, handler will be generated every time time per call
        */
        bool single_use_register_handlers = false;

        /**
         * when "single_use_register_handlers" is set to false,
         * this value will be used to determine the chance of generating a new handler
         */
        float chance_to_generate_register_handler = .3;

        /**
         * before each command is translated, there will be a chance for the machine to generate code
         * which writes to unused register space occupied by x86 context storing register
         */
        float command_chance_to_write_random = .75;

        /**
         * when enabled, the returning register (by default VTEMP) will be randomized for handlers
         * for this feature to work for VM AND x86 handlers, "single_use_vm_handlers" and "single_use_x86_handlers" must both be enabled
         * for this feature to work on register loading handlers, "single_use_register_handlers" must be enabled
         * if the conditions are not met for these cases, VTEMP/VTEMP2 will be used by default
         */
        bool randomize_returning_register = false;
    };

    using settings_ptr = std::shared_ptr<settings>;
}
