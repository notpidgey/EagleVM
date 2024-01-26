#include "virtual_machine/handlers/vm_handler_entry.h"

vm_handler_entry::vm_handler_entry(vm_register_manager* manager, vm_handler_generator* handler_generator)
            : base_instruction_virtualizer(manager, handler_generator), has_builder_hook(false), is_vm_handler(false)
{

}

code_label* vm_handler_entry::get_handler_va(reg_size size) const
{
    const auto label = supported_handlers.find(size);
    if(label != supported_handlers.end())
    {
        return label->second;
    }

    return nullptr;
}

function_container vm_handler_entry::construct_handler()
{
    for(auto size : supported_sizes)
    {
        code_label* label = container.assign_label("handler." + std::to_string(size));
        container.assign_label(label);

        construct_single(container, size);

        supported_handlers[size] = label;
    }

    return container;
}