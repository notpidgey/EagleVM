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
    for(const auto& handler : hg_.vm_handlers | std::views::values)
    {
        function_container container = handler->construct_handler();
        section.add(container);
    }

    if(randomize_handler_position) {}

    return section;
}

void vm_generator::call_vm_enter(function_container& container, code_label* target)
{
    const vm_handler_entry* vmenter = hg_.vm_handlers[MNEMONIC_VM_ENTER];
    const auto vmenter_address = vmenter->get_handler_va(bit64);

    container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_PUSH, ZLABEL(target))));

    code_label* rel_label = code_label::create("call_vm_enter_rel");
    container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREL(vmenter_address, rel_label))));
}

void vm_generator::call_vm_exit(function_container& container, code_label* target)
{
    const vm_handler_entry* vmexit = hg_.vm_handlers[MNEMONIC_VM_EXIT];
    const auto vmexit_address = vmexit->get_handler_va(bit64);

    // we are currently inside the virtual machine
    // we want to jump back to the original code for whatever reason

    container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_PUSH, ZLABEL(target))));

    code_label* rel_label = code_label::create("call_vm_exit_rel");
    container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREL(vmexit_address, rel_label))));
}

encoded_vec vm_generator::create_jump(uint32_t current, code_label* target)
{
    const uint32_t relative_jump = target->get() - (current + 5);

    zydis_encoder_request jmp = zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZIMMS(relative_jump));
    return zydis_helper::encode_request(jmp);
}

std::pair<bool, function_container> vm_generator::translate_to_virtual(const zydis_decode& decoded)
{
    vm_handler_entry* handler = hg_.vm_handlers[decoded.instruction.mnemonic];
    if(!handler)
        return { false, {} };

    return handler->translate_to_virtual(decoded);
}

std::vector<uint8_t> vm_generator::create_padding(const size_t bytes)
{
    std::vector<uint8_t> padding;
    padding.reserve(bytes);

    for(int i = 0; i < bytes; i++)
        padding.push_back(ran_device::get().gen_8());

    return padding;
}