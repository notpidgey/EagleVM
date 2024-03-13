#include "virtual_machine/handlers/handler/base_handler_entry.h"

base_handler_entry::base_handler_entry(vm_inst_regs* manager, vm_inst_handlers* handler_generator)
{
    hg_ = handler_generator;
    rm_ = manager;

    has_builder_hook = false;
    is_vm_handler = false;
}

function_container base_handler_entry::construct_handler()
{
    std::ranges::for_each(handlers, [this](handler_info& info)
    {
        container.assign_label(info.target_label);
        construct_single(container, info.instruction_width, info.operand_count);

        const char size = zydis_helper::reg_size_to_string(info.instruction_width);
        std::printf("%3c %-17s %-10zi\n", size, typeid(*this).name(), container.size());
    });

    std::printf("\n");

    return container;
}

void base_handler_entry::initialize_labels()
{
    for(auto& handler : handlers)
        handler.target_label = code_label::create();
}

void base_handler_entry::create_vm_return(function_container& container)
{
    // now that we use a virtual call stack we must pop the top address

    // mov VCSRET, [VCS]        ; pop from call stack
    // lea VCS, [VCS + 8]       ; move up the call stack pointer
    // lea VIP, [VBASE + VCSRET]  ; add rva to base
    // jmp VIP

    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VCSRET), ZMEMBD(VCS, 0, 8)));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VCS), ZMEMBD(VCS, 8, 8)));

    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBI(VBASE, VCSRET, 1, 8)));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREG(VIP)));
}

void base_handler_entry::call_vm_handler(function_container& container, code_label* jump_label)
{
    if(!jump_label)
        __debugbreak(); // jump_label should never be null

    code_label* retun_label = code_label::create();

    // lea VCS, [VCS - 8]       ; allocate space for new return address
    // mov [VCS], code_label    ; place return rva on the stack
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VCS), ZMEMBD(VCS, -8, 8)));
    container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZMEMBD(VCS, 0, 8), ZLABEL(retun_label))));

    // lea VIP, [VBASE + VCSRET]  ; add rva to base
    // jmp VIP
    container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBD(VBASE, jump_label->get(), 8))));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREG(VIP)));

    // execution after VM handler should end up here
    container.assign_label(retun_label);
}

