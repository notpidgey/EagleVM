#include "virtual_machine/vm_generator.h"
#include "util/random.h"

vm_generator::vm_generator()
{
    zydis_helper::setup_decoder();
    rm_ = new vm_register_manager();
    hg_ = new vm_handler_generator(rm_);
}

void vm_generator::init_reg_order()
{
    rm_->init_reg_order();
}

void vm_generator::init_ran_consts()
{
    hg_->setup_enc_constants();
}

section_manager vm_generator::generate_vm_handlers(bool randomize_handler_position)
{
    hg_->setup_vm_mapping();
    section_manager section;

    for(const auto& handler : hg_->v_handlers | std::views::values)
        handler->initialize_labels();

    for(const auto& handler : hg_->inst_handlers | std::views::values)
        handler->initialize_labels();

    for(const auto& handler : hg_->v_handlers | std::views::values)
    {
        function_container container = handler->construct_handler();
        section.add(container);
    }

    for(const auto& handler : hg_->inst_handlers | std::views::values)
    {
        function_container container = handler->construct_handler();
        section.add(container);
    }

    if(randomize_handler_position) {}

    return section;
}

void vm_generator::call_vm_enter(function_container& container, code_label* target)
{
    const vm_handler_entry* vmenter = hg_->v_handlers[MNEMONIC_VM_ENTER];
    const auto vmenter_address = vmenter->get_vm_handler_va(bit64);

    container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_PUSH, ZLABEL(target))));

    code_label* rel_label = code_label::create("call_vm_enter_rel");
    container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREL(vmenter_address, rel_label))));
}

void vm_generator::call_vm_exit(function_container& container, code_label* target)
{
    const vm_handler_entry* vmexit = hg_->v_handlers[MNEMONIC_VM_EXIT];
    const auto vmexit_address = vmexit->get_vm_handler_va(bit64);

    // mov VCSRET, ZLABEL(target)
    container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VCSRET), ZLABEL(target))));

    code_label* rel_label = code_label::create("call_vm_exit_rel");
    container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREL(vmexit_address, rel_label))));
}

void vm_generator::create_vm_jump(zyids_mnemonic mnemonic, function_container &container, code_label* rva_target)
{
    code_label* rel_label = code_label::create("call_vm_enter_rel");
    container.add(rel_label, [=]()
    {
        int32_t immediate_value;
        if (mnemonic == ZYDIS_MNEMONIC_JMP) {
            immediate_value = int32_t(rva_target->get()) - (int32_t(rel_label->get()) + 5);
        } else {
            bool is_short = false;
            auto diff = uint32_t(rva_target->get()) - int32_t(rel_label->get());
            if(diff <= 0xFF)
            {
                if(rel_label->get() > rva_target->get())
                    immediate_value = ~uint64_t(rel_label->get() - rva_target->get() + 1);
                else
                    immediate_value = rva_target->get() - rel_label->get() - 2;
            }
            else
            {
                if(rel_label->get() > rva_target->get())
                    immediate_value = ~uint64_t(rel_label->get() - rva_target->get() + 5);
                else
                    immediate_value = rva_target->get() - rel_label->get() -6;
            }
        }

        return zydis_helper::enc(mnemonic, zydis_eimm{ .s = immediate_value });
    });
}

encoded_vec vm_generator::create_jump(const uint32_t rva, code_label* rva_target)
{
    const uint32_t relative_jump = rva_target->get() - (rva + 5);

    zydis_encoder_request jmp = zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZIMMS(relative_jump));
    return zydis_helper::encode_request(jmp);
}

std::pair<bool, function_container> vm_generator::translate_to_virtual(const zydis_decode& decoded_instruction)
{
    inst_handler_entry* handler = hg_->inst_handlers[decoded_instruction.instruction.mnemonic];
    if(!handler)
        return { false, {} };

    return handler->translate_to_virtual(decoded_instruction);
}

std::vector<uint8_t> vm_generator::create_padding(const size_t bytes)
{
    std::vector<uint8_t> padding;
    padding.reserve(bytes);

    for(int i = 0; i < bytes; i++)
        padding.push_back(ran_device::get().gen_8());

    return padding;
}