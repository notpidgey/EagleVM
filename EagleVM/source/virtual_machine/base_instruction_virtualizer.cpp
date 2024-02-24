#include "virtual_machine/base_instruction_virtualizer.h"

#include "virtual_machine/handlers/vm_handler_generator.h"
#include "virtual_machine/vm_register_manager.h"


base_instruction_virtualizer::base_instruction_virtualizer(vm_register_manager* manager, vm_handler_generator* handler_generator)
{
    rm_ = manager;
    hg_ = handler_generator;
}

void base_instruction_virtualizer::create_vm_return(function_container& container)
{
    // now that we use a virtual call stack we must pop the top address

    // mov VCSRET, [VCS]        ; pop from call stack
    // lea VCS, [VCS + 8]       ; move up the call stack pointer
    // lea VIP, [0x14000000]    ; load base
    // lea VIP, [VIP + VCSRET]  ; add rva to base
    // jmp VIP

    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZREG(VCSRET), ZMEMBD(VCS, 0, 8)));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VCS), ZMEMBD(VCS, 8, 8)));

    code_label* rel_label = code_label::create();
    container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBD(IP_RIP, -rel_label->get(), 8))));

    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBI(VIP, VCSRET, 1, 8)));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREG(VIP)));
}

void base_instruction_virtualizer::call_vm_handler(function_container& container, code_label* jump_label)
{
    if(!jump_label)
        __debugbreak(); // jump_label should never be null

    code_label* retun_label = code_label::create();
    code_label* rel_label = code_label::create();

    // lea VCS, [VCS - 8]       ; allocate space for new return address
    // mov [VCS], code_label    ; place return rva on the stack
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VCS), ZMEMBD(VCS, -8, 8)));
    container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_MOV, ZMEMBD(VCS, 0, 8), ZLABEL(retun_label))));

    // lea VIP, [0x14000000]    ; load base
    // lea VIP, [VIP + VCSRET]  ; add rva to base
    // jmp VIP
    container.add(rel_label, RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBD(IP_RIP, -rel_label->get(), 8))));
    container.add(RECOMPILE(zydis_helper::enc(ZYDIS_MNEMONIC_LEA, ZREG(VIP), ZMEMBD(VIP, jump_label->get(), 8))));
    container.add(zydis_helper::enc(ZYDIS_MNEMONIC_JMP, ZREG(VIP)));

    // execution after VM handler should end up here
    container.assign_label(retun_label);
}

