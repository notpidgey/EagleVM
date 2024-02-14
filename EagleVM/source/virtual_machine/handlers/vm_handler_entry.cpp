#include "virtual_machine/handlers/vm_handler_entry.h"

vm_handler_entry::vm_handler_entry(vm_register_manager* manager, vm_handler_generator* handler_generator)
    : base_instruction_virtualizer(manager, handler_generator), has_builder_hook(false), is_vm_handler(false)
{
}

code_label* vm_handler_entry::get_handler_va(reg_size size) const
{
    const auto label = supported_handlers.find(size);
    if (label != supported_handlers.end())
    {
        return label->second;
    }

    return nullptr;
}

void vm_handler_entry::setup_labels()
{
    for (auto size: supported_sizes)
    {
        code_label* label = container.assign_label("handler." + std::to_string(size));
        supported_handlers[size] = label;
    }
}

function_container vm_handler_entry::construct_handler()
{
    for (auto size: supported_sizes)
    {
        code_label* label = supported_handlers[size];
        container.assign_label(label);

        construct_single(container, size);
    }

    return container;
}

void vm_handler_entry::finalize_translate_to_virtual(const zydis_decode& decoded, function_container& container)
{
    reg_size target_size = get_target_handler_size(decoded);
    code_label* target_handler = supported_handlers[target_size];

    call_vm_handler(container, target_handler);
}

reg_size vm_handler_entry::get_target_handler_size(const zydis_decode& decoded)
{
    reg_size target_size = reg_size::bit64;
    //
    // auto operand = decoded.operands[0];
    // switch (operand.type)
    // {
    //     case ZYDIS_OPERAND_TYPE_REGISTER:
    //     {
    //         auto reg = operand.reg;
    //         target_size = zydis_helper::get_reg_size(reg.value);
    //         break;
    //     }
    //     case ZYDIS_OPERAND_TYPE_MEMORY:
    //     {
    //         // theres no way to know but we can look at the next operand
    //         auto mem = operand.mem;
    //         if (decoded.operands->element_count >= 2)
    //         {
    //             operand = decoded.operands[1];
    //             if (operand.type != ZYDIS_OPERAND_TYPE_REGISTER)
    //             {
    //                 // how the hell do we find out the size then....
    //                 __debugbreak();
    //             } else
    //             {
    //                 auto reg = operand.reg;
    //                 target_size = zydis_helper::get_reg_size(reg.value);
    //             }
    //
    //             break;
    //         }
    //
    //         // fall through to a default, this should probably be overridden if something like this happens
    //     }
    //     default:
    //         __debugbreak();
    //         break;
    // }

    return reg_size(decoded.instruction.operand_width / 8);
}
