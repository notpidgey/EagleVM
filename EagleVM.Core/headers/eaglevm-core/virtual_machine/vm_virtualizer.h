#pragma once
#include "eaglevm-core/disassembler/disassembler.h"
#include "eaglevm-core/disassembler/models/basic_block.h"
#include "eaglevm-core/virtual_machine/vm_inst.h"

class vm_virtualizer
{
public:
    explicit vm_virtualizer(vm_inst* virtual_machine);
    ~vm_virtualizer();

    std::vector<function_container> virtualize_segment(segment_dasm* dasm);
    function_container virtualize_block(segment_dasm* dasm, basic_block* block);

    code_label* get_label(basic_block* block);

    static encoded_vec create_padding(size_t bytes);
    static encoded_vec create_jump(uint32_t rva, code_label* rva_target);

private:
    std::unordered_map<basic_block*, code_label*> block_labels_;
    vm_inst* vm_inst_;
    vm_inst_handlers* hg_;
    vm_inst_regs* rm_;

    void call_vm_enter(function_container& container, code_label* target);
    void call_vm_exit(function_container& container, code_label* target);
    void create_vm_jump(zyids_mnemonic mnemonic, function_container &container, code_label* rva_target);

    std::pair<bool, function_container> translate_to_virtual(const zydis_decode& decoded_instruction, uint64_t original_rva);
};