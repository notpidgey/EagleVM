#pragma once
#include "eaglevm-core/disassembler/disassembler.h"
#include "eaglevm-core/disassembler/basic_block.h"

#include "eaglevm-core/virtual_machine/vm_inst.h"

namespace eagle::virt
{
    class vm_virtualizer
    {
    public:
        explicit vm_virtualizer(vm_inst* virtual_machine);
        ~vm_virtualizer();

        std::vector<asmbl::function_container> virtualize_segment(dasm::segment_dasm* dasm);
        asmbl::function_container virtualize_block(dasm::segment_dasm* dasm, dasm::basic_block* block);

        asmbl::code_label* get_label(dasm::basic_block* block);

        static encoded_vec create_padding(size_t bytes);
        static encoded_vec create_jump(uint32_t rva, asmbl::code_label* rva_target);

    private:
        std::unordered_map<dasm::basic_block*, asmbl::code_label*> block_labels_;
        vm_inst* vm_inst_;
        vm_inst_handlers* hg_;
        vm_inst_regs* rm_;

        void call_vm_enter(asmbl::function_container& container, asmbl::code_label* target);
        void call_vm_exit(asmbl::function_container& container, asmbl::code_label* target);
        void create_vm_jump(zyids_mnemonic mnemonic, asmbl::function_container &container, asmbl::code_label* rva_target);

        std::pair<bool, asmbl::function_container> translate_to_virtual(const zydis_decode& decoded_instruction, uint64_t original_rva);
    };
}