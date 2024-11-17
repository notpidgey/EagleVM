#pragma once
#include <memory>

namespace eagle::virt::eg
{
    using settings_ptr = std::shared_ptr<struct settings>;

    struct settings
    {
        /**
        * when "single_use_x86_handlers" is set to false,
        * this value will be used to determine the chance of generating a new handler
        *
        * recommended value: 1.0
        */
        float chance_to_generate_x86_handler = 0.3;

        bool shuffle_push_order = false;
        bool shuffle_vm_gpr_order = false;
        bool shuffle_vm_xmm_order = false;
    };

    using settings_ptr = std::shared_ptr<settings>;
}
