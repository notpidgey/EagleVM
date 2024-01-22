#include "virtual_machine/vm_generator.h"
#include "util/random.h"

vm_generator::vm_generator()
{
    zydis_helper::setup_decoder();
    hg_ = vm_handler_generator(&rm_);
}

void vm_generator::init_reg_order()
{
    rm_.init_reg_order();
}

void vm_generator::init_ran_consts()
{
    hg_.setup_enc_constants();
}

section_manager vm_generator::generate_vm_handlers(bool randomize_handler_position)
{
    hg_.setup_vm_mapping();

    section_manager section;
    for (const auto& handler : hg_.vm_handlers | std::views::values)
    {
        function_container container = handler->construct_handler();
        section.add(container);
    }

    if (randomize_handler_position)
    {

    }

    return section;
}

std::vector<zydis_encoder_request> vm_generator::call_vm_enter()
{
    return {};
}

std::vector<zydis_encoder_request> vm_generator::call_vm_exit()
{
    return {};
}

std::pair<bool, std::vector<dynamic_instruction>> vm_generator::translate_to_virtual(const zydis_decode& decoded)
{
    vm_handler_entry* handler = hg_.vm_handlers[decoded.instruction.mnemonic];
    return handler->translate_to_virtual(decoded);
}

std::vector<uint8_t> vm_generator::create_padding(const size_t bytes)
{
    std::vector<uint8_t> padding;
    padding.reserve(bytes);

    for (int i = 0; i < bytes; i++)
        padding.push_back(ran_device::get().gen_8());

    return padding;
}