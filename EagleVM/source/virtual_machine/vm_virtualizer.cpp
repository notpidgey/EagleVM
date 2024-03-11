#include "virtual_machine/vm_virtualizer.h"
#include "util/random.h"

vm_virtualizer::vm_virtualizer(vm_inst* virtual_machine)
{
    zydis_helper::setup_decoder();
    vm_inst_ = virtual_machine;
}

void vm_virtualizer::call_vm_enter(function_container& container, code_label* target)
{
    const vm_handler_entry* vmenter = vm_inst_->get_handlers()->v_handlers[MNEMONIC_VM_ENTER];
    const auto vmenter_address = vmenter->get_vm_handler_va(bit64);

    container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_PUSH, ZLABEL(target))));

    code_label* rel_label = code_label::create("call_vm_enter_rel");
    container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZJMP(vmenter_address, rel_label))));
}

void vm_virtualizer::call_vm_exit(function_container& container, code_label* target)
{
    const vm_handler_entry* vmexit = vm_inst_->get_handlers()->v_handlers[MNEMONIC_VM_EXIT];
    const auto vmexit_address = vmexit->get_vm_handler_va(bit64);

    // mov VCSRET, ZLABEL(target)
    container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VCSRET), ZLABEL(target))));

    // lea VRIP, [VBASE + vmexit_address]
    container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBD(VBASE, vmexit_address->get(), 8))));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREG(VIP)));
}

void vm_virtualizer::create_vm_jump(zyids_mnemonic mnemonic, function_container &container, code_label* rva_target)
{
    code_label* rel_label = code_label::create("call_vm_enter_rel");
    container.add(rel_label, RECOMPILE(zydis_helper::enc(mnemonic, ZJMP(rva_target, rel_label))));
}

std::pair<bool, function_container> vm_virtualizer::translate_to_virtual(const zydis_decode& decoded_instruction)
{
    inst_handler_entry* handler = vm_inst_->get_handlers()->inst_handlers[decoded_instruction.instruction.mnemonic];
    if(!handler)
        return { false, {} };

    return handler->translate_to_virtual(decoded_instruction);
}

std::vector<uint8_t> vm_virtualizer::create_padding(const size_t bytes)
{
    std::vector<uint8_t> padding;
    padding.reserve(bytes);

    for(int i = 0; i < bytes; i++)
        padding.push_back(ran_device::get().gen_8());

    return padding;
}

encoded_vec vm_virtualizer::create_jump(const uint32_t rva, code_label* rva_target)
{
    zydis_encoder_request jmp = zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZIMMS(rva_target->get() - rva - 5));
    return zydis_helper::encode_request(jmp);
}