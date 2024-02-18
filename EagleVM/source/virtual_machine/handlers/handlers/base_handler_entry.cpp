#include "virtual_machine/handlers/handler/base_handler_entry.h"

base_handler_entry::base_handler_entry(vm_register_manager* manager, vm_handler_generator* handler_generator)
    : base_instruction_virtualizer(manager, handler_generator), has_builder_hook(false), is_vm_handler(false) {}

function_container base_handler_entry::construct_handler()
{
    std::ranges::for_each(handlers, [this](handler_info& info)
    {
        container.assign_label(info.target_label);
        info.construct(container, info.instruction_width);
    });

    return container;
}

void base_handler_entry::initialize_labels()
{
    for(auto& handler : handlers)
        handler.target_label = code_label::create();
}