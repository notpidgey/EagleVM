#include "virtual_machine/handlers/vm_handler_entry.h"

vm_handler_entry::vm_handler_entry()
{

}

code_label* vm_handler_entry::get_handler_va(reg_size size)
{
    auto label = supported_handlers.find(size);
    if (label != supported_handlers.end())
    {
        return label->second;
    }

    return nullptr;
}

function_container& vm_handler_entry::construct_handler()
{
    for (auto size : supported_sizes)
    {
        handle_instructions size_handler = construct_single(size);

        code_label* label = container.assign_label("handler." + std::to_string(size));
        container.add(size_handler);

        supported_handlers[size] = label;
    }

    return container;
}
