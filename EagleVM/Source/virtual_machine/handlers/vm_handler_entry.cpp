#include "virtual_machine/handlers/vm_handler_entry.h"

vm_handler_entry::vm_handler_entry(): has_builder_hook(false), is_vm_handler(false) {}

code_label* vm_handler_entry::get_handler_va(reg_size size) const
{
    auto label = supported_handlers.find(size);
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
        dynamic_instructions_vec size_handler = construct_single(size);

        code_label* label = container.assign_label("handler." + std::to_string(size));
        container.add(size_handler);

        supported_handlers[size] = label;
    }

    return container;
}

bool vm_handler_entry::hook_builder_init(const zydis_decode& decoded, dynamic_instructions_vec& instruction)
{
    return true;
}

bool vm_handler_entry::hook_builder_operand(const zydis_decode& decoded, dynamic_instructions_vec& instructions,
    int index)
{
    return true;
}

bool vm_handler_entry::hook_builder_finalize(const zydis_decode& decoded, dynamic_instructions_vec& instructions)
{
    return true;
}
