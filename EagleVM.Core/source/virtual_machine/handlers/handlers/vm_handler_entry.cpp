#include "eaglevm-core/virtual_machine/handlers/handler/vm_handler_entry.h"

code_label* vm_handler_entry::get_vm_handler_va(reg_size width, handler_override override) const
{
    const auto it = std::ranges::find_if(handlers,
        [width, override](const handler_info& h)
        {
            if(h.instruction_width == width && h.override == override)
                return true;

            return false;
        });

    if(it != handlers.end())
        return it->target_label;

    return nullptr;
}