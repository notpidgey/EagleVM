#pragma once
#include "virtual_machine/vm_inst.h"

class vm_virtualizer
{
public:
    explicit vm_virtualizer(vm_inst* virtual_machine);

    void call_vm_enter(function_container& container, code_label* target);
    void call_vm_exit(function_container& container, code_label* target);
    void create_vm_jump(zyids_mnemonic mnemonic, function_container &container, code_label* rva_target);

    std::pair<bool, function_container> translate_to_virtual(const zydis_decode& decoded_instruction);

    static encoded_vec create_padding(size_t bytes);
    static encoded_vec create_jump(uint32_t rva, code_label* rva_target);

private:
    vm_inst* vm_inst_;
};